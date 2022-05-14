#ifndef SRC_DB_DATA_SOURCE_H_

#define SRC_DB_DATA_SOURCE_H_

#include <string>

namespace db {

// Abstract Database Interface that supports transaction operations.
class DataSource {
 public:
  DataSource() = default;
  ~DataSource() = default;
  
  // Connects to the database instance.
  virtual grpc::Status Connect();

  // Begins a transaction.
  virtual grpc::Status Begin();
  
  // Commits a transaction.
  virtual grpc::Status Commit();

  // Aborts a transaction.
  virtual grpc::Status Abort();

  // Gets the value from the database with input |key|.
  // It assumes a transaction is already opened with Begin.
  // Throws error if no transaction is valid.
  virtual int64_t Get(const std::string& key);

  // Put the |value| into the database with input |key|.
  // It assumes a transaction is already opened with Begin.
  // Throws error if no transaction is valid.
  virtual void Put(const std::string& key, int64_t value);
};

}  // namespace db

#endif  // SRC_DB_DATA_SOURCE_H_