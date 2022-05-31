#include "src/cohort/cohort_server.h"

#include <fstream>

#include "absl/status/statusor.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "glog/logging.h"
#include "grpcpp/server_context.h"
#include "src/blockchain/two_phase_commit.h"
#include "src/proto/cohort.grpc.pb.h"
#include "src/utils/status_utils.h"

namespace cohort {

namespace {

using Blockchain = ::blockchain::TwoPhaseCommit;
using ::grpc::ServerContext;

template <typename Message>
bool WriteToFile(const Message& message, const std::string& path) {
  std::fstream output(path, std::ios::out | std::ios::trunc | std::ios::binary);
  return message.SerializeToOstream(&output);
}

}  // namespace

absl::Mutex& CohortServer::GetLock(const std::string& key) {
  std::unique_ptr<absl::Mutex>& lock = locks_by_key_[key];
  absl::MutexLock locks_by_key_lock(&locks_by_key_mutex_);
  if (lock == nullptr) {
    lock = std::make_unique<absl::Mutex>();
  }
  return *lock;
}

absl::Status CohortServer::AcquireWholeDbLock(
    const common::Transaction& transaction, absl::Time presumed_abort_time,
    internal::TransactionMetadata& txn_metadata) {
  bool has_put = false;
  bool has_get = false;
  for (const common::Operation& op : transaction.ops()) {
    if (op.has_put()) {
      has_put = true;
    }
    if (op.has_get()) {
      has_get = true;
    }
    if (has_put && has_get) {
      break;
    }
  }
  if (has_put) {
    if (!whole_db_mutex_.WriterLockWhenWithDeadline(absl::Condition::kTrue,
                                                    presumed_abort_time)) {
      return absl::DeadlineExceededError(
          "Could not acquire write lock for the DB before the abort deadline");
    }
    txn_metadata.has_whole_db_write_lock = true;
    return absl::OkStatus();
  }
  if (has_get) {
    if (!whole_db_mutex_.ReaderLockWhenWithDeadline(absl::Condition::kTrue,
                                                    presumed_abort_time)) {
      return absl::DeadlineExceededError(
          "Could not acquire read lock for the DB before the abort deadline");
    }
    txn_metadata.has_whole_db_read_lock = true;
  }
  return absl::OkStatus();
}

absl::Status CohortServer::AcquireDbLocks(
    const common::Transaction& transaction, absl::Time presumed_abort_time,
    internal::TransactionMetadata& txn_metadata) {
  if (!txn_metadata.db->SupportsConcurrentWrites()) {
    return AcquireWholeDbLock(transaction, presumed_abort_time, txn_metadata);
  }
  absl::flat_hash_set<std::string> write_and_readwrite_keys;
  // Excludes any keys that are written to as well. Otherwise deadlocks could
  // occur if we wait for both the read and write locks.
  absl::flat_hash_set<std::string> readonly_keys;
  for (const common::Operation& op : transaction.ops()) {
    if (op.has_put()) {
      readonly_keys.erase(op.put().key());
      write_and_readwrite_keys.emplace(op.put().key());
    }
    if (op.has_get() && !write_and_readwrite_keys.contains(op.get().key())) {
      readonly_keys.emplace(op.get().key());
    }
  }

  for (const auto& read_key : readonly_keys) {
    if (!GetLock(read_key).ReaderLockWhenWithDeadline(absl::Condition::kTrue,
                                                      presumed_abort_time)) {
      return absl::DeadlineExceededError(
          absl::StrCat("Could not acquire read lock for ", read_key,
                       " before the abort deadline"));
    }
    txn_metadata.read_lock_keys.push_back(read_key);
  }
  for (const auto& write_key : write_and_readwrite_keys) {
    if (!GetLock(write_key).WriterLockWhenWithDeadline(absl::Condition::kTrue,
                                                       presumed_abort_time)) {
      return absl::DeadlineExceededError(
          absl::StrCat("Could not acquire write lock for ", write_key,
                       " before the abort deadline"));
    }
    txn_metadata.write_lock_keys.push_back(write_key);
  }
  return absl::OkStatus();
}

absl::Status CohortServer::ProcessOperationInDb(
    const common::Operation& op, internal::TransactionMetadata& txn_metadata) {
  if (op.has_get()) {
    int64_t value;
    RETURN_IF_ERROR(txn_metadata.db->Get(op.get().key(), value));
    common::GetResponse* get_response =
        txn_metadata.response.mutable_committed_response()->add_get_responses();
    *get_response->mutable_get() = op.get();
    *get_response->mutable_namespace_() = op.namespace_();
    get_response->mutable_value()->set_int64_value(value);
    return absl::OkStatus();
  }
  if (op.has_put()) {
    if (op.put().value().has_constant_value()) {
      return txn_metadata.db->Put(
          op.put().key(), op.put().value().constant_value().int64_value());
    }
    int64_t value;
    absl::Status get_status = txn_metadata.db->Get(op.put().key(), value);
    if (get_status.code() == absl::NotFoundError("").code() &&
        op.put().value().relative_value().has_default_value()) {
      value = op.put().value().relative_value().default_value().int64_value();
    } else if (!get_status.ok()) {
      return get_status;
    }
    return txn_metadata.db->Put(
        op.put().key(),
        value +
            op.put().value().relative_value().relative_value().int64_value());
  }
  // No-op.
  return absl::OkStatus();
}

absl::Status CohortServer::ProcessTransactionInDb(
    const common::Transaction& transaction, absl::Time presumed_abort_time,
    internal::TransactionMetadata& txn_metadata) {
  RETURN_IF_ERROR(
      AcquireDbLocks(transaction, presumed_abort_time, txn_metadata));
  if (txn_metadata.write_lock_keys.empty() &&
      !txn_metadata.has_whole_db_write_lock) {
    RETURN_IF_ERROR(txn_metadata.db->BeginReadOnly());
  } else {
    RETURN_IF_ERROR(txn_metadata.db->Begin());
  }
  for (const common::Operation& op : transaction.ops()) {
    RETURN_IF_ERROR(ProcessOperationInDb(op, txn_metadata));
  }
  return absl::OkStatus();
}

blockchain::VotingDecision CohortServer::WaitForBlockchainDecision(
    const std::string& transaction_id, absl::Time presumed_abort_time) {
  bool waited_until_presumed_abort_time = false;
  while (true) {
    const blockchain::VotingDecision decision =
        blockchain_->GetVotingDecision(transaction_id)
            .value_or(blockchain::VotingDecision::VOTING_DECISION_PENDING);
    if (decision != blockchain::VotingDecision::VOTING_DECISION_PENDING &&
        decision != blockchain::VotingDecision::VOTING_DECISION_UNKNOWN) {
      return decision;
    }
    VLOG(1) << "Waiting for blockchain decision";
    if (waited_until_presumed_abort_time) {
      std::this_thread::sleep_for(
          absl::ToChronoMicroseconds(absl::Now() - presumed_abort_time));
    } else {
      std::this_thread::sleep_until(absl::ToChronoTime(presumed_abort_time));
      waited_until_presumed_abort_time = true;
    }
  }
}

void CohortServer::AbortTransaction(const std::string& transaction_id,
                                    int cohort_index,
                                    absl::optional<absl::Status> abort_status) {
  common::AbortReason abort_reason;
  if (abort_status.has_value()) {
    LOG(INFO) << "Aborting due to " << abort_status.value();
    // It's okay if it fails since it will auto-abort at the presumed abort
    // time.
    blockchain_
        ->Vote(transaction_id, cohort_index, blockchain::Ballot::BALLOT_ABORT)
        .IgnoreError();
    switch (abort_status->code()) {
      case absl::StatusCode::kDeadlineExceeded:
        abort_reason = common::ABORT_REASON_PRESUMED_ABORT_TIMESTAMP_REACHED;
        break;
      case absl::StatusCode::kNotFound:
        abort_reason = common::ABORT_REASON_OPERATION_FOR_NON_EXISTENT_VALUE;
        break;
      default:
        abort_reason = common::ABORT_REASON_UNSPECIFIED;
    }
  }
  metadata_by_transaction_id_[transaction_id].response.set_aborted_response(
      abort_reason);
  // TODO(benjmarks22): Maybe add retry logic here?
  metadata_by_transaction_id_[transaction_id].db->Abort().IgnoreError();
  ReleaseLocksAndDeleteMetadata(transaction_id);
}

void CohortServer::CommitTransaction(const std::string& transaction_id) {
  // The database must commit the transaction at this point, so the only
  // acceptable failures are transient ones (e.g. deadline exceeded). Thus we
  // retry until it succeeds.
  while (true) {
    absl::Status commit_status =
        metadata_by_transaction_id_[transaction_id].db->Commit();
    if (commit_status.ok()) {
      break;
    }
    LOG(WARNING) << "Failed to commit transaction " << commit_status
                 << ". Retrying";
  }
  // This is necessary if it's a write only transaction to ensure the response
  // indicates that it committed.
  metadata_by_transaction_id_[transaction_id]
      .response.mutable_committed_response();
  ReleaseLocksAndDeleteMetadata(transaction_id);
}

absl::Status CohortServer::PersistTransaction(
    const PrepareTransactionRequest& request) {
  const std::string& response_path =
      absl::StrCat(db_txn_response_dir_, "/response_", request.transaction_id(),
                   ".binarypb");
  if (!WriteToFile(metadata_by_transaction_id_[request.transaction_id()]
                       .response.committed_response(),
                   response_path)) {
    return absl::InternalError(
        "Failed to write transaction responses to disk.");
  }
  const std::string& request_path = absl::StrCat(
      db_txn_response_dir_, "/request_", request.transaction_id(), ".binarypb");
  if (!WriteToFile(request, request_path)) {
    return absl::InternalError("Failed to write transaction request to disk.");
  }
  return absl::OkStatus();
}

void CohortServer::ProcessTransaction(
    const PrepareTransactionRequest& request) {
  internal::TransactionMetadata& metadata =
      metadata_by_transaction_id_[request.transaction_id()];
  metadata.response.mutable_pending_response();
  metadata.db = db_transaction_adapter_creator_();
  const absl::Time presumed_abort_time =
      absl::FromUnixSeconds(request.config().presumed_abort_time().seconds()) +
      absl::Nanoseconds(request.config().presumed_abort_time().nanos());

  const absl::Status db_status = ProcessTransactionInDb(
      request.transaction(), presumed_abort_time, metadata);
  if (!db_status.ok()) {
    AbortTransaction(request.transaction_id(), request.cohort_index(),
                     db_status);
    return;
  }
  if (request.only_cohort()) {
    CommitTransaction(request.transaction_id());
    return;
  }

  const absl::Status persist_status = PersistTransaction(request);
  if (!persist_status.ok()) {
    AbortTransaction(request.transaction_id(), request.cohort_index(),
                     persist_status);
    return;
  }

  // It's not safe to abort if there's an error here since the blockchain may
  // have accepted our commit vote and decided to commit the transaction.
  // TODO(benjmarks22): Add retry logic here.
  blockchain_
      ->Vote(request.transaction_id(), request.cohort_index(),
             blockchain::Ballot::BALLOT_COMMIT)
      .IgnoreError();
  if (WaitForBlockchainDecision(request.transaction_id(),
                                presumed_abort_time) ==
      blockchain::VotingDecision::VOTING_DECISION_COMMIT) {
    CommitTransaction(request.transaction_id());
  } else {
    AbortTransaction(request.transaction_id(), request.cohort_index(),
                     absl::nullopt);
  }
}

grpc::Status CohortServer::PrepareTransaction(
    ServerContext* /*context*/, const PrepareTransactionRequest* request,
    PrepareTransactionResponse* /*response*/) {
  const PrepareTransactionRequest& request_non_pointer = *request;
  thread_pool_.push_task([this, request_non_pointer]() {
    return ProcessTransaction(request_non_pointer);
  });
  return grpc::Status::OK;
}

grpc::Status CohortServer::GetTransactionResult(
    ServerContext* /*context*/, const GetTransactionResultRequest* request,
    GetTransactionResultResponse* response) {
  if (final_response_by_transaction_id_.contains(request->transaction_id())) {
    *response = final_response_by_transaction_id_[request->transaction_id()];
  } else {
    response->mutable_pending_response();
  }
  return grpc::Status::OK;
}

void CohortServer::ReleaseLocksAndDeleteMetadata(
    const std::string& transaction_id) {
  for (const auto& write_key :
       metadata_by_transaction_id_[transaction_id].write_lock_keys) {
    GetLock(write_key).WriterUnlock();
  }
  for (const auto& read_key :
       metadata_by_transaction_id_[transaction_id].read_lock_keys) {
    GetLock(read_key).ReaderUnlock();
  }
  if (metadata_by_transaction_id_[transaction_id].has_whole_db_write_lock) {
    whole_db_mutex_.WriterUnlock();
  }
  if (metadata_by_transaction_id_[transaction_id].has_whole_db_read_lock) {
    whole_db_mutex_.ReaderUnlock();
  }
  // Should be faster than copying the response and the metadata one gets
  // deleted right after.
  final_response_by_transaction_id_[transaction_id].Swap(
      &metadata_by_transaction_id_[transaction_id].response);
  metadata_by_transaction_id_.erase(transaction_id);
}

}  // namespace cohort