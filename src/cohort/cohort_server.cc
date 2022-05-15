#include "src/cohort/cohort_server.h"

#include "absl/status/statusor.h"
#include "grpcpp/server_context.h"
#include "src/proto/cohort.grpc.pb.h"
#include "src/utils/grpc_utils.h"

namespace cohort {

using grpc::ServerContext;

absl::Status CohortServer::StartWork(const PrepareTransactionRequest &request) {
  // TODO(benjmarks22): Create a fiber and start the transaction.
  results_by_transaction_id_[request.transaction_id()]
      .mutable_pending_response();
  return absl::OkStatus();
}

grpc::Status CohortServer::PrepareTransaction(
    grpc::ServerContext * /*context*/, const PrepareTransactionRequest *request,
    PrepareTransactionResponse * /*response*/) {
  return utils::FromAbslStatus(StartWork(*request));
}

grpc::Status CohortServer::GetTransactionResult(
    grpc::ServerContext * /*context*/,
    const GetTransactionResultRequest *request,
    GetTransactionResultResponse *response) {
  *response = results_by_transaction_id_[request->transaction_id()];
  return grpc::Status::OK;
}

}  // namespace cohort