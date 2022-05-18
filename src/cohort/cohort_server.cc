#include "src/cohort/cohort_server.h"

#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "grpcpp/server_context.h"
#include "src/proto/cohort.grpc.pb.h"
#include "src/utils/grpc_utils.h"

namespace cohort {

namespace {

using ::grpc::ServerContext;

std::chrono::time_point<std::chrono::system_clock> FromProtoTimestamp(
    const google::protobuf::Timestamp &timestamp) {
  return absl::ToChronoTime(absl::FromUnixSeconds(timestamp.seconds()) +
                            absl::Nanoseconds(timestamp.nanos()));
}

}  // namespace

GetTransactionResultResponse CohortServer::Begin(
    const PrepareTransactionRequest & /*request*/) {
  auto response = GetTransactionResultResponse();
  response.mutable_pending_response();
  // TODO(benjmarks22): Interact with the database to set the response.
  // TODO(benjmarks22): Vote in the blockchain based on the database response.
  return response;
}

grpc::Status CohortServer::PrepareTransaction(
    ServerContext * /*context*/, const PrepareTransactionRequest *request,
    PrepareTransactionResponse * /*response*/) {
  results_by_transaction_id_[request->transaction_id()].config =
      request->config();
  results_by_transaction_id_[request->transaction_id()].future =
      thread_pool_.submit([this, request]() { return Begin(*request); });
  return grpc::Status::OK;
}

grpc::Status CohortServer::GetTransactionResult(
    ServerContext * /*context*/, const GetTransactionResultRequest *request,
    GetTransactionResultResponse *response) {
  const auto time =
      FromProtoTimestamp(results_by_transaction_id_[request->transaction_id()]
                             .config.presumed_abort_time());
  if (results_by_transaction_id_[request->transaction_id()].future.wait_until(
          time) == std::future_status::ready) {
    *response =
        results_by_transaction_id_[request->transaction_id()].future.get();
  }
  return grpc::Status::OK;
}

}  // namespace cohort