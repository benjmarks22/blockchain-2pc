#ifndef SRC_DB_DATA_SOURCE_H_

#define SRC_DB_DATA_SOURCE_H_

#include <string>

#include "absl/status/status.h"

namespace db {

// Abstract Database Interface that supports transaction operations.
class DataSource {
 public:
  DataSource() = default;
  ~DataSource() = default;

  // Begins a transaction.
  virtual absl::Status Begin() = 0;

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

#endif  // SRC_DB_DATA_SOURCE_H_