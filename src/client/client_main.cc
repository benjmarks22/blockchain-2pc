#include <fcntl.h>

#include <fstream>
#include <iostream>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/status/statusor.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "glog/logging.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/text_format.h"
#include "grpc/grpc.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"
#include "src/client/client.h"
#include "src/client/client.pb.h"
#include "src/proto/common.pb.h"
#include "src/proto/coordinator.grpc.pb.h"

ABSL_FLAG(std::string, coordinator_port, "58000",
          "Port to send requests to the coordinator");
ABSL_FLAG(std::string, request_file_prefix, "",
          "Prefix of filename with requests to send to the coordinator");

struct RunInfo {
  absl::flat_hash_set<std::string> cohort_addresses;
  std::vector<std::pair<coordinator::CommitAtomicTransactionRequest, bool>>
      requests;
};

coordinator::CommitAtomicTransactionRequest CreateRequest(
    const std::string& client_transaction_id) {
  coordinator::CommitAtomicTransactionRequest request;
  request.set_client_transaction_id(client_transaction_id);
  timespec presumed_abort_timespec =
      absl::ToTimespec(absl::Now() + absl::Seconds(5));
  request.mutable_config()->mutable_presumed_abort_time()->set_seconds(
      presumed_abort_timespec.tv_sec);
  request.mutable_config()->mutable_presumed_abort_time()->set_nanos(
      presumed_abort_timespec.tv_nsec);
  return request;
}

void AddGetOp(coordinator::CommitAtomicTransactionRequest& request,
              const std::string& address, const std::string& key) {
  common::Operation* get_op = request.mutable_transaction()->add_ops();
  get_op->mutable_namespace_()->set_address(address);
  get_op->mutable_get()->set_key(key);
}

void AddPutOp(coordinator::CommitAtomicTransactionRequest& request,
              const std::string& address, const std::string& key,
              int64_t value) {
  common::Operation* put_op = request.mutable_transaction()->add_ops();
  put_op->mutable_namespace_()->set_address(address);
  put_op->mutable_put()->set_key(key);
  put_op->mutable_put()
      ->mutable_value()
      ->mutable_constant_value()
      ->set_int64_value(value);
}

void AddRelativePutOp(coordinator::CommitAtomicTransactionRequest& request,
                      const std::string& address, const std::string& key,
                      int64_t relative_value) {
  common::Operation* put_op = request.mutable_transaction()->add_ops();
  put_op->mutable_namespace_()->set_address(address);
  put_op->mutable_put()->set_key(key);
  put_op->mutable_put()
      ->mutable_value()
      ->mutable_relative_value()
      ->mutable_relative_value()
      ->set_int64_value(relative_value);
}

std::string GetRandomAddress() {
  const static auto* addresses = new std::vector<std::string>{
      "0.0.0.0:60000", "0.0.0.0:60001", "0.0.0.0:60002", "0.0.0.0:60003",
      "0.0.0.0:60004", "0.0.0.0:60005", "0.0.0.0:60006", "0.0.0.0:60007",
      "0.0.0.0:60008", "0.0.0.0:60009",
  };
  return (*addresses)[rand() % addresses->size()];
}

std::string GetRandomKey() {
  std::vector<char> chars;
  for (size_t i = 0; i < 3; ++i) {
    chars.push_back('a' + rand() % 26);
  }
  return std::string(chars.begin(), chars.end());
}

std::string GetRandomKey(const absl::flat_hash_set<std::string>& keys) {
  auto iterator = keys.begin();
  std::advance(iterator, rand() % keys.size());
  return *iterator;
}

int64_t GetRandomValue() {
  // Ranges from -10 to 40.
  return rand() % 50 - 10;
}

void AddOperation(
    const client::Operation& op,
    coordinator::CommitAtomicTransactionRequest& request, RunInfo& run_info,
    absl::flat_hash_map<std::string, absl::flat_hash_set<std::string>>&
        put_keys_by_address) {
  std::string address;
  if (op.address_type_case() == client::Operation::kRandomAddress) {
    address = GetRandomAddress();
  } else {
    address = op.address();
  }
  run_info.cohort_addresses.emplace(address);
  std::string key;
  if (op.key_type_case() == client::Operation::kRandomKey) {
    key = GetRandomKey();
  } else if (op.key_type_case() == client::Operation::kRandomPutKey) {
    key = GetRandomKey(put_keys_by_address[address]);
  } else {
    key = op.key();
  }
  if (op.type() == client::Operation::OP_TYPE_PUT) {
    put_keys_by_address[address].emplace(key);
    if (op.value_type_case() == client::Operation::kRandomRelativeValue) {
      AddRelativePutOp(request, address, key, GetRandomValue());
    } else if (op.value_type_case() == client::Operation::kRelativeValue) {
      AddRelativePutOp(request, address, key, op.relative_value());
    } else if (op.value_type_case() == client::Operation::kRandomValue) {
      AddPutOp(request, address, key, GetRandomValue());
    } else {
      AddPutOp(request, address, key, op.value());
    }
  } else {
    AddGetOp(request, address, key);
  }
}

void AddTransaction(
    const client::SimpleTransaction& transaction, RunInfo& run_info,
    absl::flat_hash_map<std::string, absl::flat_hash_set<std::string>>&
        put_keys_by_address) {
  coordinator::CommitAtomicTransactionRequest request;
  if (transaction.has_time_before_abort()) {
    // The current time will be added just before the request.
    request.mutable_config()->mutable_presumed_abort_time()->set_seconds(
        transaction.time_before_abort().seconds());
    request.mutable_config()->mutable_presumed_abort_time()->set_nanos(
        transaction.time_before_abort().nanos());
  }
  for (const client::Operation& op : transaction.op()) {
    for (int64_t i = 0; i < (op.repeat_count() != 0 ? op.repeat_count() : 1);
         ++i) {
      AddOperation(op, request, run_info, put_keys_by_address);
    }
  }
  request.set_client_transaction_id(absl::CEscape(request.SerializeAsString()));
  run_info.requests.emplace_back(request, transaction.async());
}

absl::StatusOr<RunInfo> CreateRunInfoFromFile(const std::string& filename) {
  client::SimpleTransactions transactions;
  int fd = open(filename.data(), O_RDONLY);
  google::protobuf::io::FileInputStream fstream(fd);
  if (!google::protobuf::TextFormat::Parse(&fstream, &transactions)) {
    return absl::InvalidArgumentError(
        "File could not be parsed as transactions");
  }
  absl::flat_hash_map<std::string, absl::flat_hash_set<std::string>>
      put_keys_by_address;
  RunInfo run_info;
  for (const client::SimpleTransaction& transaction :
       transactions.initial_transaction()) {
    AddTransaction(transaction, run_info, put_keys_by_address);
  }
  for (int64_t i = 0; i < transactions.cycle_length(); ++i) {
    for (const client::SimpleTransaction& transaction :
         transactions.cycle_transaction()) {
      AddTransaction(transaction, run_info, put_keys_by_address);
    }
  }
  return run_info;
}

void LogResponseOrStatus(const client::ResponseOrStatus& response_or_status) {
  if (absl::holds_alternative<grpc::Status>(response_or_status)) {
    const auto& status = absl::get<grpc::Status>(response_or_status);
    LOG(INFO) << status.error_code() << " " << status.error_message();
  } else {
    LOG(INFO) << absl::get<coordinator::GetTransactionResultResponse>(
                     response_or_status)
                     .DebugString();
  }
}

void SendRequests(
    const std::string& coordinator_port,
    std::vector<std::pair<coordinator::CommitAtomicTransactionRequest, bool>>&
        requests) {
  client::Client client(
      grpc::CreateChannel(absl::StrCat("0.0.0.0:", coordinator_port),
                          grpc::InsecureChannelCredentials()));
  client.WaitForCoordinator();
  std::vector<std::future<client::ResponseOrStatus>> futures;
  bool first_request = true;
  for (std::pair<coordinator::CommitAtomicTransactionRequest, bool>& request :
       requests) {
    if (!first_request) {
      std::this_thread::sleep_for(absl::ToChronoSeconds(absl::Seconds(5)));
    }
    first_request = false;
    if (request.first.config().has_presumed_abort_time()) {
      request.first.mutable_config()
          ->mutable_presumed_abort_time()
          ->set_seconds(request.first.config().presumed_abort_time().seconds() +
                        absl::ToUnixSeconds(absl::Now()));
    }
    if (request.second) {
      futures.push_back(client.CommitAsync(request.first));
    } else {
      LogResponseOrStatus(client.CommitSync(request.first));
    }
  }
  for (auto& future : futures) {
    LogResponseOrStatus(future.get());
  }
}

int main(int argc, char** argv) {
  try {
    absl::ParseCommandLine(argc, argv);
    absl::StatusOr<RunInfo> run_info = CreateRunInfoFromFile(
        absl::StrCat("src/client/requests/",
                     absl::GetFlag(FLAGS_request_file_prefix), ".textproto"));
    if (!run_info.ok()) {
      LOG(ERROR) << run_info.status();
      exit(EXIT_FAILURE);
    }
    SendRequests(absl::GetFlag(FLAGS_coordinator_port), run_info->requests);
  } catch (std::exception& exc) {
    std::cerr << exc.what() << std::endl;
    exit(EXIT_FAILURE);
  }
  return (EXIT_SUCCESS);
}