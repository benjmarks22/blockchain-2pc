#include "src/db/lmdb_data_source.h"

#include <sys/stat.h>

#include "absl/strings/str_format.h"

namespace db {

namespace {

// Maximum size of the db in memory.
constexpr uint64_t k1GbMapsize = 1UL * 1024UL * 1024UL * 1024UL;

// Read & Write access as owner.
constexpr mdb_mode_t kFileOpenMode = (S_IRUSR | S_IWUSR);

}  // namespace

LMDBDataSource::LMDBDataSource(std::string_view db_path) : db_path_(db_path) {
  Connect();
}

void LMDBDataSource::Connect() {
  env_ = std::make_unique<lmdb::env>(lmdb::env::create());
  env_->set_mapsize(k1GbMapsize);
  env_->open(db_path_.c_str(), 0, kFileOpenMode);
}

absl::Status LMDBDataSource::Begin() {
  if (txn_ != nullptr) {
    return absl::FailedPreconditionError(
        "Cannot open another transaction while a transaction hasn't "
        "commited or aborted yet.");
  }
  txn_ = std::make_unique<lmdb::txn>(lmdb::txn::begin(*env_));
  dbi_ = std::make_unique<lmdb::dbi>(lmdb::dbi::open(*txn_, nullptr));
  return absl::OkStatus();
}

absl::Status LMDBDataSource::Commit() {
  if (txn_ == nullptr) {
    return absl::FailedPreconditionError(
        "No valid transaction to Commit. Please Begin() first.");
  }
  txn_->commit();
  ReleaseTransaction();
  return absl::OkStatus();
}

absl::Status LMDBDataSource::Abort() {
  if (txn_ == nullptr) {
    return absl::FailedPreconditionError(
        "No valid transaction to Abort. Please Begin() first.");
  }
  txn_->abort();
  ReleaseTransaction();
  return absl::OkStatus();
}

absl::Status LMDBDataSource::Get(const std::string& key,
                                 int64_t& output_value) {
  if (txn_ == nullptr) {
    return absl::FailedPreconditionError(
        "No valid transaction available. Please Begin() first.");
  }
  if (!dbi_->get(*txn_, key, output_value)) {
    return absl::InternalError("Get failed.");
  }
  return absl::OkStatus();
}

absl::Status LMDBDataSource::Put(const std::string& key, int64_t value) {
  if (txn_ == nullptr) {
    return absl::FailedPreconditionError(
        "No valid transaction available. Please Begin() first.");
  }
  if (!dbi_->put(*txn_, key, value)) {
    return absl::InternalError("Put failed.");
  }
  return absl::OkStatus();
}

void LMDBDataSource::ReleaseTransaction() {
  txn_.release();
  dbi_.release();
}

}  // namespace db