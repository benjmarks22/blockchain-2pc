#include <iostream>

#include "glog/logging.h"
#include "grpc/grpc.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"
#include "src/client/client.h"
#include "src/proto/common.pb.h"
#include "src/proto/coordinator.grpc.pb.h"

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

client::ResponseOrStatus PutSingleNamespace(client::Client& client) {
  coordinator::CommitAtomicTransactionRequest request_put =
      CreateRequest("put_single");
  AddPutOp(request_put, "0.0.0.0:50051", "a", 3);
  return client.CommitSync(request_put);
}

client::ResponseOrStatus PutTwoNamespaces(client::Client& client) {
  coordinator::CommitAtomicTransactionRequest request_put =
      CreateRequest("put_two");
  AddPutOp(request_put, "0.0.0.0:50051", "a", 3);
  AddPutOp(request_put, "0.0.0.0:50053", "b", 5);
  return client.CommitSync(request_put);
}

client::ResponseOrStatus GetSingleNamespace(client::Client& client) {
  coordinator::CommitAtomicTransactionRequest request_get =
      CreateRequest("get_single");
  AddGetOp(request_get, "0.0.0.0:50051", "a");
  return client.CommitSync(request_get);
}

client::ResponseOrStatus GetTwoNamespaces(client::Client& client) {
  coordinator::CommitAtomicTransactionRequest request_get =
      CreateRequest("get_two");
  AddGetOp(request_get, "0.0.0.0:50051", "a");
  AddGetOp(request_get, "0.0.0.0:50053", "b");
  return client.CommitSync(request_get);
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

int main(int /*argc*/, char** /*argv*/) {
  try {
    client::Client client(grpc::CreateChannel(
        "localhost:50052", grpc::InsecureChannelCredentials()));
    LogResponseOrStatus(PutSingleNamespace(client));
    LogResponseOrStatus(GetSingleNamespace(client));
  } catch (std::exception& exc) {
    std::cerr << exc.what() << std::endl;
    exit(EXIT_FAILURE);
  }
  return (EXIT_SUCCESS);
}