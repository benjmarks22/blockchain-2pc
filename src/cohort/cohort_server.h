#ifndef SRC_COHORT_COHORT_SERVER_H_

#define SRC_COHORT_COHORT_SERVER_H_

#include <thread>

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "grpcpp/server_context.h"
#include "src/proto/cohort.grpc.pb.h"
#include "thread_pool.hpp"

namespace cohort {

namespace internal {
struct ConfigAndResponse {
  common::TransactionConfig config;
  std::future<GetTransactionResultResponse> future;
};
}  // namespace internal

class CohortServer : public Cohort::Service {
 public:
  explicit CohortServer(uint num_db_threads) : thread_pool_(num_db_threads) {}

  grpc::Status PrepareTransaction(
      grpc::ServerContext *context, const PrepareTransactionRequest *request,
      PrepareTransactionResponse *response) override;

  grpc::Status GetTransactionResult(
      grpc::ServerContext *context, const GetTransactionResultRequest *request,
      GetTransactionResultResponse *response) override;

 private:
  static GetTransactionResultResponse Begin(
      const PrepareTransactionRequest &request);

  absl::flat_hash_map<const std::string, internal::ConfigAndResponse>
      results_by_transaction_id_;

  thread_pool thread_pool_;
};

}  // namespace cohort

#endif  // SRC_COHORT_COHORT_SERVER_H_