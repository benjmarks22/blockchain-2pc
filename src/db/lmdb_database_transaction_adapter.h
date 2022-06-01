#ifndef SRC_DB_LMDB_DATABASE_TRANSACTION_ADAPTER_H_

#define SRC_DB_LMDB_DATABASE_TRANSACTION_ADAPTER_H_

#include <memory>
#include <string>

#include "lmdbxx/lmdb++.h"
#include "src/db/database_transaction_adapter.h"

namespace db {

// Database Interface that supports transaction operations based on
// LMDB primitives.
// NOTICE:
//   LMDB only support 1 open read-write txn and 0+ read-only txns.
//   Two 2+ read-write txn will block the process from proceeding.
class LMDBDatabaseTransactionAdapter : public DatabaseTransactionAdapter {
 public:
  explicit LMDBDatabaseTransactionAdapter(std::string_view db_path);

  ~LMDBDatabaseTransactionAdapter() = default;

  [[nodiscard]] bool SupportsConcurrentWrites() const final { return false; }

  // Begins a read-write transaction.
  absl::Status Begin() final;

  // Begins a read-only transaction.
  absl::Status BeginReadOnly() final;

  // Commits a transaction.
  absl::Status Commit() final;

  // Aborts a transaction.
  absl::Status Abort() final;

  // Gets the value from the database with input |key|.
  // It assumes a transaction is already opened with Begin.
  // Returns FailedPrecondition error if no transaction is valid.
  absl::Status Get(const std::string& key, int64_t& output_value) final;

  // Puts the |value| into the database with input |key|.
  // It assumes a transaction is already opened with Begin.
  // Returns FailedPrecondition error if no transaction is valid.
  absl::Status Put(const std::string& key, int64_t value) final;

 private:
  // Connects to the database instance in the class construnctor.
  // Raises expection if it fails. E.g. directory represented by
  // |db_path_| doesn't exist.
  void Connect() final;

  // Releases the txn_ and db_ objects held by unique_ptr.
  void ReleaseTransaction();

  const std::string db_path_;
  bool is_readonly_ = false;
  std::unique_ptr<lmdb::env> env_ = nullptr;
  std::unique_ptr<lmdb::txn> txn_ = nullptr;
  std::unique_ptr<lmdb::dbi> dbi_ = nullptr;
};

}  // namespace db

#endif  // SRC_DB_LMDB_DATABASE_TRANSACTION_ADAPTER_H_