#ifndef SRC_COHORT_COHORT_SERVER_H_

#define SRC_COHORT_COHORT_SERVER_H_

#include <thread>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/statusor.h"
#include "grpcpp/server_context.h"
#include "src/db/database_transaction_adapter.h"
#include "src/proto/cohort.grpc.pb.h"
#include "thread_pool.hpp"

namespace cohort {

namespace internal {
struct TransactionMetadata {
  GetTransactionResultResponse response;
  std::unique_ptr<db::DatabaseTransactionAdapter> db;
  std::vector<std::string> read_lock_keys;
  std::vector<std::string> write_lock_keys;
};
}  // namespace internal

class CohortServer : public Cohort::Service {
 public:
  CohortServer(uint num_db_threads, const std::string& db_txn_response_dir,
               std::function<std::unique_ptr<db::DatabaseTransactionAdapter>()>
                   db_transaction_adapter_creator)
      : thread_pool_(num_db_threads),
        db_txn_response_dir_(db_txn_response_dir),
        db_transaction_adapter_creator_(db_transaction_adapter_creator) {}

  grpc::Status PrepareTransaction(
      grpc::ServerContext* context, const PrepareTransactionRequest* request,
      PrepareTransactionResponse* response) override;

  grpc::Status GetTransactionResult(
      grpc::ServerContext* context, const GetTransactionResultRequest* request,
      GetTransactionResultResponse* response) override;

 private:
  absl::Mutex& GetLock(const std::string& key);

  absl::Status AcquireDbLocks(const common::Transaction& transaction,
                              absl::Time presumed_abort_time,
                              internal::TransactionMetadata& txn_metadata);

  void ProcessTransaction(const PrepareTransactionRequest& request);

  void AbortTransaction(const std::string& transaction_id, int cohort_index,
                        absl::optional<absl::Status> abort_status);

  void CommitTransaction(const std::string& transaction_id);

  absl::Status PersistTransaction(const PrepareTransactionRequest& request);

  absl::Status ProcessOperationInDb(
      const common::Operation& op, internal::TransactionMetadata& txn_metadata);

  absl::Status ProcessTransactionInDb(
      const common::Transaction& transaction, absl::Time presumed_abort_time,
      internal::TransactionMetadata& txn_metadata);

  void ReleaseLocksAndDeleteMetadata(const std::string& transaction_id);

  absl::flat_hash_map<const std::string, internal::TransactionMetadata>
      metadata_by_transaction_id_;

  absl::flat_hash_map<const std::string, GetTransactionResultResponse>
      final_response_by_transaction_id_;

  thread_pool thread_pool_;
  const std::string db_txn_response_dir_;
  std::function<std::unique_ptr<db::DatabaseTransactionAdapter>()>
      db_transaction_adapter_creator_;

  absl::Mutex locks_by_key_mutex_;
  absl::flat_hash_map<std::string, std::unique_ptr<absl::Mutex>> locks_by_key_;
};

}  // namespace cohort

#endif  // SRC_COHORT_COHORT_SERVER_H_