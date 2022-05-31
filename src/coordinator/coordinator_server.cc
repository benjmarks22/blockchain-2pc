#include "src/coordinator/coordinator_server.h"

#include <future>
#include <sstream>
#include <string>

#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "glog/logging.h"
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

using CohortStub = ::cohort::Cohort::StubInterface;
using ::common::Namespace;
using ::coordinator::internal::SubTransaction;
using ::coordinator::internal::TransactionMetadata;
using ::grpc::ClientContext;
using ::grpc::ServerContext;

bool RequiresBlockchain(const std::vector<SubTransaction> &sub_transactions) {
  return sub_transactions.size() > 1;
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

std::vector<SubTransaction> SplitClientTransaction(
    const common::Transaction &original_transaction,
    bool sort_sub_transactions) {
  absl::flat_hash_map<std::string, SubTransaction> transactions_by_namespace;
  for (const auto &operation : original_transaction.ops()) {
    SubTransaction &transaction_with_namespace =
        transactions_by_namespace[operation.namespace_().address()];
    transaction_with_namespace.namespace_ = operation.namespace_();
    *transaction_with_namespace.transaction.add_ops() = operation;
  }
  std::vector<SubTransaction> sub_transactions;
  sub_transactions.reserve(transactions_by_namespace.size());
  for (const auto &transaction_with_namespace : transactions_by_namespace) {
    sub_transactions.push_back(transaction_with_namespace.second);
  }
  if (sort_sub_transactions) {
    std::sort(sub_transactions.begin(), sub_transactions.end(),
              [](const SubTransaction &t1, const SubTransaction &t2) {
                return t1.namespace_.address() < t2.namespace_.address();
              });
  }
  return sub_transactions;
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
  metadata.config = config;
  const std::vector<SubTransaction> sub_transactions =
      SplitClientTransaction(request->transaction(), SortCohortRequests());
  if (sub_transactions.empty()) {
    return grpc::Status::OK;
  }
  if (RequiresBlockchain(sub_transactions)) {
    absl::Status blockchain_status = StartVoting(
        transaction_id, config.presumed_abort_time(), sub_transactions.size());
    if (!blockchain_status.ok()) {
      return utils::FromAbslStatus(blockchain_status,
                                   "Failed to start voting in blockchain");
    }
  }
  SendCohortPrepareRequests(transaction_id, sub_transactions, *context);
  return grpc::Status::OK;
}

void CoordinatorServer::SendCohortPrepareRequests(
    const std::string &transaction_id,
    const std::vector<SubTransaction> &sub_transactions,
    const ServerContext &context) {
  TransactionMetadata &metadata = metadata_by_transaction_[transaction_id];
  cohort::PrepareTransactionRequest prepare_request;
  prepare_request.set_transaction_id(transaction_id);
  *prepare_request.mutable_config() = metadata.config;
  if (sub_transactions.size() == 1) {
    prepare_request.set_only_cohort(true);
    metadata.single_cohort_namespace = sub_transactions[0].namespace_;
  }
  size_t cohort_index = 0;
  std::vector<std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<
      cohort::PrepareTransactionResponse>>>
      rpcs;
  rpcs.reserve(sub_transactions.size());
  for (const SubTransaction &sub_transaction : sub_transactions) {
    if (sub_transactions.size() != 1) {
      prepare_request.set_cohort_index(cohort_index);
    }
    *prepare_request.mutable_transaction() = sub_transaction.transaction;
    metadata.cohort_namespaces.push_back(sub_transaction.namespace_);
    // If this is the last cohort, then it may have been sent to all cohorts.
    if (cohort_index == sub_transactions.size() - 1) {
      metadata.possibly_sent_to_all_cohorts = true;
    }

    PrepareCohortTransaction(transaction_id, sub_transaction.namespace_,
                             prepare_request, context);
    ++cohort_index;
  }
}

absl::Status CoordinatorServer::StartVoting(
    const std::string &transaction_id,
    const google::protobuf::Timestamp &presumed_abort_time,
    size_t num_cohorts) {
  const time_t abort_time =
      absl::ToTimeT(absl::FromUnixSeconds(presumed_abort_time.seconds()) +
                    absl::Nanoseconds(presumed_abort_time.nanos()));
  return blockchain_->StartVoting(transaction_id, abort_time, num_cohorts);
}

absl::StatusOr<blockchain::VotingDecision> CoordinatorServer::GetVotingDecision(
    const std::string &transaction_id) {
  return blockchain_->GetVotingDecision(transaction_id);
}

absl::Time CoordinatorServer::Now() { return absl::Now(); }

void CoordinatorServer::PrepareCohortTransaction(
    const std::string &transaction_id, const Namespace &namespace_,
    const cohort::PrepareTransactionRequest &request,
    const grpc::ServerContext &context) {
  metadata_by_transaction_[transaction_id].contexts.push_back(
      ClientContext::FromServerContext(context));
  cohort::PrepareTransactionResponse response;
  GetCohortStub(namespace_)
      .PrepareTransaction(
          metadata_by_transaction_[transaction_id].contexts.back().get(),
          request, &response);
}

grpc::Status CoordinatorServer::FinishPrepareCohortTransaction(
    std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<
        cohort::PrepareTransactionResponse>> &async_response,
    cohort::PrepareTransactionResponse &response) {
  grpc::Status status;
  async_response->Finish(&response, &status,
                         /*tag=*/static_cast<void *>(&async_response));
  return status;
}

std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<
    cohort::GetTransactionResultResponse>>
CoordinatorServer::AsyncGetResultsFromCohort(
    const Namespace &namespace_,
    const cohort::GetTransactionResultRequest &request,
    ClientContext &context) {
  return GetCohortStub(namespace_)
      .AsyncGetTransactionResult(&context, request, /*cq=*/nullptr);
}

grpc::Status CoordinatorServer::FinishAsyncGetResultsFromCohort(
    std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<
        cohort::GetTransactionResultResponse>> &async_response,
    cohort::GetTransactionResultResponse &response) {
  grpc::Status status;
  async_response->Finish(&response, &status,
                         /*tag=*/static_cast<void *>(&async_response));
  return status;
}

grpc::Status CoordinatorServer::GetResultsFromCohort(
    const Namespace &namespace_,
    const cohort::GetTransactionResultRequest &request,
    grpc::ClientContext &context,
    cohort::GetTransactionResultResponse &response) {
  return GetCohortStub(namespace_)
      .GetTransactionResult(&context, request, &response);
}

CohortStub &CoordinatorServer::GetCohortStub(const Namespace &namespace_) {
  std::unique_ptr<CohortStub> &cohort_stub =
      cohort_by_namespace_[namespace_.address()];
  if (!cohort_stub) {
    cohort_stub = cohort::Cohort::NewStub(grpc::CreateChannel(
        namespace_.address(), grpc::InsecureChannelCredentials()));
  }
  return *cohort_stub;
}

grpc::Status CoordinatorServer::UpdateResponseForSingleCohortTransaction(
    const std::string &transaction_id, const ServerContext &context) {
  GetTransactionResultResponse &response =
      response_by_transaction_[transaction_id].response;
  cohort::GetTransactionResultRequest cohort_request;
  cohort_request.set_transaction_id(transaction_id);
  std::unique_ptr<ClientContext> transaction_result_context =
      ClientContext::FromServerContext(context);
  cohort::GetTransactionResultResponse cohort_response;
  const Namespace &namespace_ =
      metadata_by_transaction_[transaction_id].single_cohort_namespace.value();
  grpc::Status status = GetResultsFromCohort(
      namespace_, cohort_request, *transaction_result_context, cohort_response);
  if (!status.ok()) {
    return grpc::Status(
        status.error_code(),
        absl::StrCat("Failed to get cohort results: ", status.error_message()));
  }
  if (cohort_response.has_aborted_response()) {
    *response.mutable_aborted_response()->add_namespaces() = namespace_;
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
    response.mutable_pending_response();
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
  for (const auto &namespace_ :
       metadata_by_transaction_[transaction_id].cohort_namespaces) {
    if (metadata_by_transaction_[transaction_id]
            .cohorts_already_responded.contains(namespace_.address())) {
      ++num_committed_responses;
    } else {
      transaction_result_contexts.push_back(
          ClientContext::FromServerContext(context));
      // TODO(benjmarks22): Figure out how to get async requests to work
      // properly. For now, use synchronous requests.
      async_responses_by_namespace[namespace_.address()] =
          AsyncGetResultsFromCohort(namespace_, cohort_request,
                                    *transaction_result_contexts.back());
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
          .cohorts_already_responded.emplace(namespace_and_response.first);
    }
  }
  if (num_committed_responses ==
      metadata_by_transaction_[transaction_id].cohort_namespaces.size()) {
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
  if (metadata.decision ==
          blockchain::VotingDecision::VOTING_DECISION_PENDING ||
      metadata.decision ==
          blockchain::VotingDecision::VOTING_DECISION_UNKNOWN) {
    const auto decision_or_status =
        GetVotingDecision(request->global_transaction_id());
    if (!decision_or_status.ok()) {
      return utils::FromAbslStatus(decision_or_status.status(),
                                   "Failed to get blockchain decision");
    }
    metadata.decision = decision_or_status.value();
  }
  switch (metadata.decision) {
    case blockchain::VotingDecision::VOTING_DECISION_UNKNOWN:
    case blockchain::VotingDecision::VOTING_DECISION_PENDING:
      response->mutable_pending_response();
      break;
    case blockchain::VotingDecision::VOTING_DECISION_ABORT:
      UpdateAbortedResponseFromCohorts(request->global_transaction_id(),
                                       *context);
      *response =
          response_by_transaction_[request->global_transaction_id()].response;
      break;
    case blockchain::VotingDecision::VOTING_DECISION_COMMIT:
      UpdateCommittedResponseFromCohorts(request->global_transaction_id(),
                                         *context);
      *response =
          response_by_transaction_[request->global_transaction_id()].response;
      break;
  }
  return grpc::Status::OK;
}

}  // namespace coordinator