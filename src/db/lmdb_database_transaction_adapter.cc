#include "src/db/lmdb_database_transaction_adapter.h"

#include <sys/stat.h>

#include "absl/strings/str_format.h"

namespace db {

namespace {

// Maximum size of the db in memory.
constexpr uint64_t k1GbMapsize = 1UL * 1024UL * 1024UL * 1024UL;

// Read & Write access as owner.
constexpr mdb_mode_t kFileOpenMode = (S_IRUSR | S_IWUSR);

}  // namespace

LMDBDatabaseTransactionAdapter::LMDBDatabaseTransactionAdapter(
    std::string_view db_path)
    : db_path_(db_path) {
  Connect();
}

void LMDBDatabaseTransactionAdapter::Connect() {
  env_ = std::make_unique<lmdb::env>(lmdb::env::create());
  env_->set_mapsize(k1GbMapsize);
  env_->open(db_path_.c_str(), 0, kFileOpenMode);
}

absl::Status LMDBDatabaseTransactionAdapter::Begin() {
  if (txn_ != nullptr) {
    return absl::FailedPreconditionError(
        "Cannot open another transaction while a transaction hasn't "
        "commited or aborted yet.");
  }
  is_readonly_ = false;
  txn_ = std::make_unique<lmdb::txn>(lmdb::txn::begin(*env_));
  dbi_ = std::make_unique<lmdb::dbi>(lmdb::dbi::open(*txn_, nullptr));
  return absl::OkStatus();
}

absl::Status LMDBDatabaseTransactionAdapter::BeginReadOnly() {
  if (txn_ != nullptr) {
    return absl::FailedPreconditionError(
        "Cannot open another transaction while a transaction hasn't "
        "commited or aborted yet.");
  }
  is_readonly_ = true;
  txn_ =
      std::make_unique<lmdb::txn>(lmdb::txn::begin(*env_, nullptr, MDB_RDONLY));
  dbi_ = std::make_unique<lmdb::dbi>(lmdb::dbi::open(*txn_, nullptr));
  return absl::OkStatus();
}

absl::Status LMDBDatabaseTransactionAdapter::Commit() {
  if (txn_ == nullptr) {
    return absl::FailedPreconditionError(
        "No valid transaction to Commit. Please Begin() first.");
  }
  txn_->commit();
  ReleaseTransaction();
  return absl::OkStatus();
}

absl::Status LMDBDatabaseTransactionAdapter::Abort() {
  if (txn_ == nullptr) {
    return absl::FailedPreconditionError(
        "No valid transaction to Abort. Please Begin() first.");
  }
  txn_->abort();
  ReleaseTransaction();
  return absl::OkStatus();
}

absl::Status LMDBDatabaseTransactionAdapter::Get(const std::string &key,
                                                 int64_t &output_value) {
  if (txn_ == nullptr) {
    return absl::FailedPreconditionError(
        "No valid transaction available. Please Begin() first.");
  }
  // Need to use lmdb::val for the key to make it able to match the put.
  // Need to use ldmb::val for the value to make it use the right dbi.get
  // method.
  lmdb::val val;
  if (!dbi_->get(*txn_, lmdb::val(key), val)) {
    return absl::InternalError("Get failed.");
  }
  output_value = *val.data<int64_t>();
  return absl::OkStatus();
}

absl::Status LMDBDatabaseTransactionAdapter::Put(const std::string &key,
                                                 int64_t value) {
  if (txn_ == nullptr) {
    return absl::FailedPreconditionError(
        "No valid transaction available. Please Begin() first.");
  }
  if (is_readonly_) {
    return absl::FailedPreconditionError(
        "Cannot call Put with read only transaction. "
        "Please Abort() or Commit() the current transaction "
        "and call Begin() instead.");
  }
  // Need to use lmdb::val for the key to make it able to match the get.
  // Need to use ldmb::val for the value to make it use the right dbi.put
  // method.
  lmdb::val val{&value, sizeof(int64_t)};
  if (!dbi_->put(*txn_, lmdb::val(key), val)) {
    return absl::InternalError("Put failed.");
  }
  return absl::OkStatus();
}

void LMDBDatabaseTransactionAdapter::ReleaseTransaction() {
  txn_.reset(nullptr);
  dbi_.reset(nullptr);
}

}  // namespace db