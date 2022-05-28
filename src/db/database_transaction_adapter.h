#ifndef SRC_DB_DATABASE_TRANSACTION_ADAPTER_H_

#define SRC_DB_DATABASE_TRANSACTION_ADAPTER_H_

#include <string>

#include "absl/status/status.h"

namespace db {

// Abstract Database Interface that supports transaction operations.
// It does not need to worry about locking keys. It is expected that the caller
// ensures concurrent transactions do not modify keys used in other put/get
// transactions.
class DatabaseTransactionAdapter {
 public:
  DatabaseTransactionAdapter() = default;
  ~DatabaseTransactionAdapter() = default;

  // Returns true if the underlying database supports multiple concurrent
  // writes. If it does not support concurrent writes, the caller must ensure
  // only one write transaction is in progress at any given time.
  [[nodiscard]] virtual /*static*/ bool SupportsConcurrentWrites() const = 0;

  // Begins a transaction.
  virtual absl::Status Begin() = 0;

  // Begins a read-only transaction.
  virtual absl::Status BeginReadOnly() = 0;

  // Commits a transaction.
  virtual absl::Status Commit() = 0;

  // Aborts a transaction.
  virtual absl::Status Abort() = 0;

  // Gets the value from the database with input |key|.
  // It assumes a transaction is already opened with Begin.
  // Returns FailedPrecondition error if no transaction is valid.
  virtual absl::Status Get(const std::string& key, int64_t& output_value) = 0;

  // Put the |value| into the database with input |key|.
  // It assumes a transaction is already opened with Begin.
  // Returns FailedPrecondition error if no transaction is valid.
  virtual absl::Status Put(const std::string& key, int64_t value) = 0;

 private:
  // Connects to the database instance.
  virtual void Connect() = 0;
};

}  // namespace db

#endif  // SRC_DB_DATABASE_TRANSACTION_ADAPTER_H_