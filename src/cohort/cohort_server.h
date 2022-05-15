#ifndef SRC_COHORT_COHORT_SERVER_H_

#define SRC_COHORT_COHORT_SERVER_H_

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "grpcpp/server_context.h"
#include "src/proto/cohort.grpc.pb.h"

namespace cohort {

class CohortServer : public Cohort::Service {
 public:
  grpc::Status PrepareTransaction(
      grpc::ServerContext *context, const PrepareTransactionRequest *request,
      PrepareTransactionResponse *response) override;

  grpc::Status GetTransactionResult(
      grpc::ServerContext *context, const GetTransactionResultRequest *request,
      GetTransactionResultResponse *response) override;

 private:
  absl::Status StartWork(const PrepareTransactionRequest &request);

  absl::flat_hash_map<const std::string, GetTransactionResultResponse>
      results_by_transaction_id_;
};

}  // namespace cohort

#endif  // SRC_COHORT_COHORT_SERVER_H_