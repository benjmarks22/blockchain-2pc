#include "src/coordinator/coordinator_server.h"

#include <future>
#include <sstream>
#include <string>

#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "grpc/grpc.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"
#include "grpcpp/server_context.h"
#include "openssl/sha.h"
#include "src/blockchain/two_phase_commit.h"
#include "src/proto/cohort.grpc.pb.h"
#include "src/proto/common.pb.h"
#include "src/proto/coordinator.grpc.pb.h"
#include "src/utils/status_utils.h"

namespace coordinator {

namespace {

using Blockchain = ::blockchain::TwoPhaseCommit;
using CohortStub = ::cohort::Cohort::StubInterface;
using ::common::Namespace;
using ::grpc::ClientContext;
using ::grpc::ServerContext;

enum TransactionType {
  // Not an actual type.
  UNSET_TRANSACTION,
  READ_ONLY_TRANSACTION,
  WRITE_ONLY_TRANSACTION,
  READ_WRITE_TRANSACTION
};

struct TransactionWithNamespace {
  Namespace ns;
  common::Transaction transaction;
  TransactionType transaction_type;
};

struct SplitTransaction {
  std::vector<TransactionWithNamespace> transactions;
  TransactionType transaction_type;
};

bool RequiresBlockchain(const SplitTransaction &split_transaction) {
  return split_transaction.transactions.size() > 1;
}

bool HashSha256(absl::string_view input, unsigned char *md) {
  SHA256_CTX context;
  if (SHA256_Init(&context) == 0) {
    return false;
  }

  if (SHA256_Update(&context, input.data(), input.length()) == 0) {
    return false;
  }

  if (SHA256_Final(md, &context) == 0) {
    return false;
  }

  return true;
}

absl::StatusOr<common::TransactionConfig> ComputeFinalConfig(
    const common::TransactionConfig &client_config, absl::Time current_time,
    absl::Duration default_transaction_duration) {
  absl::Time presumed_abort_time =
      client_config.has_presumed_abort_time()
          ? absl::FromUnixSeconds(
                client_config.presumed_abort_time().seconds()) +
                absl::Nanoseconds(client_config.presumed_abort_time().nanos())
          : current_time + default_transaction_duration;
  if (presumed_abort_time < current_time) {
    return absl::InvalidArgumentError("Presumed abort time is in the past");
  }
  common::TransactionConfig final_config;
  timespec presumed_abort_timespec = absl::ToTimespec(presumed_abort_time);
  final_config.mutable_presumed_abort_time()->set_seconds(
      presumed_abort_timespec.tv_sec);
  final_config.mutable_presumed_abort_time()->set_nanos(
      presumed_abort_timespec.tv_nsec);
  return final_config;
}

absl::StatusOr<const std::string> GetTransactionId(
    const std::string &client_transaction_id,
    const std::string &client_address) {
  std::array<unsigned char, 32> hash_value;
  if (!HashSha256(absl::StrCat(client_transaction_id, "~", client_address),
                  hash_value.data())) {
    return absl::UnknownError(
        "Could not hash client transaction id and address");
  }
  std::stringstream ss;
  for (const auto &hash_byte : hash_value) {
    ss << absl::StrCat(absl::Hex(hash_byte, absl::kZeroPad2));
  }
  return ss.str();
}

SplitTransaction SplitClientTransaction(
    const common::Transaction &original_transaction) {
  absl::flat_hash_map<std::string, TransactionWithNamespace>
      transactions_by_namespace;
  bool has_writes = false;
  bool has_reads = false;
  for (const auto &operation : original_transaction.ops()) {
    TransactionWithNamespace &transaction_with_namespace =
        transactions_by_namespace[operation.namespace_().address()];
    transaction_with_namespace.ns = operation.namespace_();
    *transaction_with_namespace.transaction.add_ops() = operation;
    if (operation.has_put()) {
      has_writes = true;
    } else {
      has_reads = true;
    }
    switch (transaction_with_namespace.transaction_type) {
      case UNSET_TRANSACTION:
        transaction_with_namespace.transaction_type =
            operation.has_put() ? WRITE_ONLY_TRANSACTION
                                : READ_ONLY_TRANSACTION;
        break;
      case READ_ONLY_TRANSACTION:
        transaction_with_namespace.transaction_type =
            operation.has_put() ? READ_WRITE_TRANSACTION
                                : READ_ONLY_TRANSACTION;
        break;
      case WRITE_ONLY_TRANSACTION:
        transaction_with_namespace.transaction_type =
            operation.has_put() ? WRITE_ONLY_TRANSACTION
                                : READ_WRITE_TRANSACTION;
        break;
      case READ_WRITE_TRANSACTION:
        break;
    }
  }
  std::vector<TransactionWithNamespace> split_transactions;
  split_transactions.reserve(transactions_by_namespace.size());
  for (const auto &transaction_with_namespace : transactions_by_namespace) {
    split_transactions.push_back(transaction_with_namespace.second);
  }
  std::sort(split_transactions.begin(), split_transactions.end(),
            [](const TransactionWithNamespace &t1,
               const TransactionWithNamespace &t2) {
              return t1.ns.address() < t2.ns.address();
            });
  return SplitTransaction{
      .transactions = split_transactions,
      .transaction_type = has_reads ? (has_writes ? READ_WRITE_TRANSACTION
                                                  : READ_ONLY_TRANSACTION)
                                    : WRITE_ONLY_TRANSACTION};
}
}  // namespace

grpc::Status CoordinatorServer::CommitAtomicTransaction(
    ServerContext *context, const CommitAtomicTransactionRequest *request,
    CommitAtomicTransactionResponse *response) {
  absl::StatusOr<std::string> transaction_id_or_status =
      GetTransactionId(request->client_transaction_id(), context->peer());
  if (!transaction_id_or_status.ok()) {
    return utils::FromAbslStatus(transaction_id_or_status.status(),
                                 "Failed to create transaction id");
  }
  const std::string transaction_id = transaction_id_or_status.value();
  response->set_global_transaction_id(transaction_id);
  TransactionMetadata &metadata = metadata_by_transaction_[transaction_id];
  // If it was already sent to all the cohorts, don't prepare the transaction
  // again since it may have been completed and may not be idempotent. If
  // clients want to try again after an aborted transaction, they should use a
  // new client_transaction_id.
  if (metadata.possibly_sent_to_all_cohorts) {
    *response->mutable_config() = metadata.config;
    return grpc::Status::OK;
  }
  const absl::StatusOr<common::TransactionConfig> config_or_status =
      ComputeFinalConfig(request->config(), Now(),
                         default_presumed_abort_duration_);
  if (!config_or_status.ok()) {
    return utils::FromAbslStatus(config_or_status.status(),
                                 "Failed to compute config: ");
  }
  const common::TransactionConfig &config = config_or_status.value();
  *response->mutable_config() = config;
  const SplitTransaction split_transaction =
      SplitClientTransaction(request->transaction());
  if (split_transaction.transactions.empty()) {
    return grpc::Status(grpc::INVALID_ARGUMENT, "No operations found");
  }
  if (RequiresBlockchain(split_transaction)) {
    absl::Status block_chain_status =
        StartVoting(transaction_id, config.presumed_abort_time(),
                    split_transaction.transactions.size());
    if (!block_chain_status.ok()) {
      return utils::FromAbslStatus(block_chain_status,
                                   "Failed to start voting in blockchain");
    }
  }
  cohort::PrepareTransactionRequest prepare_request;
  prepare_request.set_transaction_id(transaction_id);
  *prepare_request.mutable_config() = config;
  if (split_transaction.transactions.size() == 1) {
    prepare_request.set_only_cohort(true);
    metadata.single_cohort_namespace = split_transaction.transactions[0].ns;
  }
  size_t cohort_index = 0;
  for (const TransactionWithNamespace &transaction_with_namespace :
       split_transaction.transactions) {
    if (split_transaction.transactions.size() != 1) {
      prepare_request.set_cohort_index(cohort_index);
    }
    *prepare_request.mutable_transaction() =
        transaction_with_namespace.transaction;
    if (transaction_with_namespace.transaction_type != WRITE_ONLY_TRANSACTION) {
      metadata.get_cohort_namespaces.push_back(transaction_with_namespace.ns);
    }
    // If this is the last cohort, then it may have been sent to all cohorts.
    if (cohort_index == split_transaction.transactions.size() - 1) {
      metadata.possibly_sent_to_all_cohorts = true;
    }
    // TODO(benjmarks22): Retry failed RPCs.
    // For now the transactions with failed RPCs will abort at the blockchain
    // at the presumed abort time.
    PrepareCohortTransaction(transaction_id, transaction_with_namespace.ns,
                             prepare_request, *context);
    ++cohort_index;
  }
  return grpc::Status::OK;
}

absl::Status CoordinatorServer::StartVoting(
    const std::string &transaction_id,
    const google::protobuf::Timestamp &presumed_abort_time,
    size_t num_cohorts) {
  const time_t abort_time =
      absl::ToTimeT(absl::FromUnixSeconds(presumed_abort_time.seconds()) +
                    absl::Nanoseconds(presumed_abort_time.nanos()));
  return Blockchain::StartVoting(transaction_id, abort_time, num_cohorts);
}

absl::StatusOr<Blockchain::VotingDecision> CoordinatorServer::GetVotingDecision(
    const std::string &transaction_id) {
  return Blockchain::GetVotingDecision(transaction_id);
}

absl::Time CoordinatorServer::Now() { return absl::Now(); }

void CoordinatorServer::PrepareCohortTransaction(
    const std::string &transaction_id, const Namespace &ns,
    const cohort::PrepareTransactionRequest &request,
    const grpc::ServerContext &context) {
  metadata_by_transaction_[transaction_id].contexts.push_back(
      ClientContext::FromServerContext(context));
  GetCohortStub(ns).AsyncPrepareTransaction(
      metadata_by_transaction_[transaction_id].contexts.back().get(), request,
      /*cq = */ nullptr);
}

std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<
    cohort::GetTransactionResultResponse>>
CoordinatorServer::AsyncGetResultsFromCohort(
    const Namespace &ns, const cohort::GetTransactionResultRequest &request,
    ClientContext &context) {
  return GetCohortStub(ns).AsyncGetTransactionResult(&context, request,
                                                     /*cq=*/nullptr);
}

grpc::Status CoordinatorServer::FinishAsyncGetResultsFromCohort(
    std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<
        cohort::GetTransactionResultResponse>> &async_response,
    cohort::GetTransactionResultResponse &response) {
  grpc::Status status;
  async_response->Finish(&response, &status, /*tag=*/nullptr);
  return status;
}

CohortStub &CoordinatorServer::GetCohortStub(const Namespace &ns) {
  std::unique_ptr<CohortStub> &cohort_stub = cohort_by_namespace_[ns.address()];
  if (!cohort_stub) {
    cohort_stub = cohort::Cohort::NewStub(
        grpc::CreateChannel(ns.address(), grpc::InsecureChannelCredentials()));
  }
  return *cohort_stub;
}

grpc::Status CoordinatorServer::UpdateResponseForSingleCohortTransaction(
    const std::string &transaction_id, const ServerContext &context) {
  GetTransactionResultResponse &response =
      response_by_transaction_[transaction_id].response;
  cohort::GetTransactionResultRequest cohort_request;
  cohort_request.set_transaction_id(transaction_id);
  std::vector<std::unique_ptr<ClientContext>> transaction_result_contexts;
  transaction_result_contexts.push_back(
      ClientContext::FromServerContext(context));
  cohort::GetTransactionResultResponse cohort_response;
  const Namespace &ns =
      metadata_by_transaction_[transaction_id].single_cohort_namespace.value();
  auto async_response = AsyncGetResultsFromCohort(
      ns, cohort_request, *transaction_result_contexts.back());
  grpc::Status status =
      FinishAsyncGetResultsFromCohort(async_response, cohort_response);
  if (!status.ok()) {
    return status;
  }
  if (cohort_response.has_aborted_response()) {
    *response.mutable_aborted_response()->add_namespaces() = ns;
    response.mutable_aborted_response()->set_reason(
        cohort_response.aborted_response());
  } else if (cohort_response.has_committed_response()) {
    response.mutable_committed_response()
        ->mutable_response()
        ->mutable_get_responses()
        ->Add(cohort_response.committed_response().get_responses().begin(),
              cohort_response.committed_response().get_responses().end());
    response.mutable_committed_response()->set_complete(true);
  } else {
    return grpc::Status::OK;
  }
  CleanUpTransactionMetadata(transaction_id);
  return grpc::Status::OK;
}

void CoordinatorServer::UpdateAbortedResponseFromCohorts(
    const std::string &transaction_id, const ServerContext & /*context*/) {
  // TODO(benjmarks22): Set aborted reason and aborted namespaces.
  response_by_transaction_[transaction_id].response.mutable_aborted_response();
  CleanUpTransactionMetadata(transaction_id);
}

void CoordinatorServer::UpdateCommittedResponseFromCohorts(
    const std::string &transaction_id, const ServerContext &context) {
  GetTransactionResultResponse &response =
      response_by_transaction_[transaction_id].response;
  cohort::GetTransactionResultRequest cohort_request;
  cohort_request.set_transaction_id(transaction_id);
  std::vector<std::unique_ptr<ClientContext>> transaction_result_contexts;
  absl::flat_hash_map<std::string,
                      std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<
                          cohort::GetTransactionResultResponse>>>
      async_responses_by_namespace;
  size_t num_committed_responses = 0;
  for (const auto &ns :
       metadata_by_transaction_[transaction_id].get_cohort_namespaces) {
    if (metadata_by_transaction_[transaction_id]
            .get_cohorts_already_responded.contains(ns.address())) {
      ++num_committed_responses;
    } else {
      transaction_result_contexts.push_back(
          ClientContext::FromServerContext(context));
      async_responses_by_namespace[ns.address()] = AsyncGetResultsFromCohort(
          ns, cohort_request, *transaction_result_contexts.back());
    }
  }
  for (auto &namespace_and_response : async_responses_by_namespace) {
    cohort::GetTransactionResultResponse cohort_response;
    grpc::Status status = FinishAsyncGetResultsFromCohort(
        namespace_and_response.second, cohort_response);
    if (status.ok() && cohort_response.has_committed_response()) {
      ++num_committed_responses;
      response.mutable_committed_response()
          ->mutable_response()
          ->mutable_get_responses()
          ->Add(cohort_response.committed_response().get_responses().begin(),
                cohort_response.committed_response().get_responses().end());
      metadata_by_transaction_[transaction_id]
          .get_cohorts_already_responded.emplace(namespace_and_response.first);
    }
  }
  if (num_committed_responses ==
      metadata_by_transaction_[transaction_id].get_cohort_namespaces.size()) {
    response.mutable_committed_response()->set_complete(true);
    CleanUpTransactionMetadata(transaction_id);
  }
}

void CoordinatorServer::CleanUpTransactionMetadata(
    const std::string &transaction_id) {
  response_by_transaction_[transaction_id].complete = true;
  metadata_by_transaction_.erase(transaction_id);
}

grpc::Status CoordinatorServer::GetTransactionResult(
    ServerContext *context, const GetTransactionResultRequest *request,
    GetTransactionResultResponse *response) {
  // If we already computed the response, return it.
  if (response_by_transaction_[request->global_transaction_id()].complete) {
    *response =
        response_by_transaction_[request->global_transaction_id()].response;
    return grpc::Status::OK;
  }
  // If we don't have metadata for the transaction, we can't ask the cohorts
  // because we don't know who they are.
  if (!metadata_by_transaction_.contains(request->global_transaction_id())) {
    return grpc::Status(grpc::NOT_FOUND, "Could not find transaction");
  }
  TransactionMetadata &metadata =
      metadata_by_transaction_[request->global_transaction_id()];
  if (metadata.single_cohort_namespace.has_value()) {
    grpc::Status status = UpdateResponseForSingleCohortTransaction(
        request->global_transaction_id(), *context);
    if (status.ok()) {
      *response =
          response_by_transaction_[request->global_transaction_id()].response;
    }
    return status;
  }
  if (metadata.decision == Blockchain::VotingDecision::PENDING) {
    const auto decision_or_status =
        GetVotingDecision(request->global_transaction_id());
    if (!decision_or_status.ok()) {
      return utils::FromAbslStatus(decision_or_status.status(),
                                   "Failed to get blockchain decision");
    }
    metadata.decision = decision_or_status.value();
  }
  switch (metadata.decision) {
    case Blockchain::VotingDecision::PENDING:
      response->mutable_pending_response();
      break;
    case Blockchain::VotingDecision::ABORT:
      UpdateAbortedResponseFromCohorts(request->global_transaction_id(),
                                       *context);
      *response =
          response_by_transaction_[request->global_transaction_id()].response;
      break;
    case Blockchain::VotingDecision::COMMIT:
      UpdateCommittedResponseFromCohorts(request->global_transaction_id(),
                                         *context);
      *response =
          response_by_transaction_[request->global_transaction_id()].response;
      break;
  }
  return grpc::Status::OK;
}

}  // namespace coordinator