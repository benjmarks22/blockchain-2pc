#include "src/db/lmdb_database_transaction_adapter.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace db {

namespace {

TEST(LMDBDatabaseTransactionAdapter, ConnectSuccessValidPath) {
  EXPECT_NO_THROW(LMDBDatabaseTransactionAdapter db_txn_adapter("/tmp"));
}

TEST(LMDBDatabaseTransactionAdapterTest, ConnectFailWithInvalidPath) {
  EXPECT_THROW(LMDBDatabaseTransactionAdapter db_txn_adapter("invalid db path"),
               lmdb::runtime_error);
}

TEST(LMDBDatabaseTransactionAdapter, TransactionOps) {
  LMDBDatabaseTransactionAdapter db_txn_adapter("/tmp");

  // Test: Begin then Commit both succeed.
  absl::Status status = db_txn_adapter.Begin();
  EXPECT_EQ(status.code(), absl::StatusCode::kOk) << status.ToString();

  status = db_txn_adapter.Commit();
  EXPECT_EQ(status.code(), absl::StatusCode::kOk) << status.ToString();

  // Test: Commit or Abort after a Commit returns FailedPrecondition error.
  status = db_txn_adapter.Commit();
  EXPECT_EQ(status.code(), absl::StatusCode::kFailedPrecondition)
      << status.ToString();

  status = db_txn_adapter.Abort();
  EXPECT_EQ(status.code(), absl::StatusCode::kFailedPrecondition)
      << status.ToString();
}

TEST(LMDBDatabaseTransactionAdapter, SingleTxnPutThenGet) {
  LMDBDatabaseTransactionAdapter db_txn_adapter("/tmp");

  const std::string key = "key";
  const int64_t put_value = 123;

  // Test: Put value succeed.
  absl::Status status = db_txn_adapter.Begin();
  ASSERT_EQ(status.code(), absl::StatusCode::kOk) << status.ToString();

  EXPECT_EQ(db_txn_adapter.Put(key, put_value).code(), absl::StatusCode::kOk);

  status = db_txn_adapter.Commit();
  EXPECT_EQ(status.code(), absl::StatusCode::kOk) << status.ToString();

  // Test: Get value succeed.
  status = db_txn_adapter.Begin();
  ASSERT_EQ(status.code(), absl::StatusCode::kOk) << status.ToString();

  int64_t got_value;
  EXPECT_EQ(db_txn_adapter.Get(key, got_value).code(), absl::StatusCode::kOk);

  status = db_txn_adapter.Commit();
  EXPECT_EQ(status.code(), absl::StatusCode::kOk) << status.ToString();
  EXPECT_EQ(got_value, put_value);
}

TEST(LMDBDatabaseTransactionAdapter, ParallelTxnsPutAndGet) {
  const std::string key1 = "key1";
  const std::string key2 = "key2";

  LMDBDatabaseTransactionAdapter db_txn_adapter1("/tmp");

  // Txn1 to Put `key1` and `key2`.
  ASSERT_EQ(db_txn_adapter1.Begin().code(), absl::StatusCode::kOk);
  ASSERT_EQ(db_txn_adapter1.Put(key1, 1).code(), absl::StatusCode::kOk);
  ASSERT_EQ(db_txn_adapter1.Put(key2, 2).code(), absl::StatusCode::kOk);
  ASSERT_EQ(db_txn_adapter1.Commit().code(), absl::StatusCode::kOk);

  LMDBDatabaseTransactionAdapter db_txn_adapter2("/tmp");
  LMDBDatabaseTransactionAdapter db_txn_adapter3("/tmp");
  ASSERT_EQ(db_txn_adapter2.Begin().code(), absl::StatusCode::kOk);
  ASSERT_EQ(db_txn_adapter3.Begin().code(), absl::StatusCode::kOk);

  int64_t txn2_got_value;
  int64_t txn3_got_value;

  // db_txn_adapter2 Gets key2 then Puts key1
  // db_txn_adapter3 Puts key2 then Gets key1
  // Both adapters do not commit.
  EXPECT_EQ(db_txn_adapter2.Get(key2, txn2_got_value).code(),
            absl::StatusCode::kOk);
  EXPECT_EQ(db_txn_adapter3.Put(key2, 7).code(), absl::StatusCode::kOk);
  EXPECT_EQ(db_txn_adapter2.Put(key1, 5).code(), absl::StatusCode::kOk);
  EXPECT_EQ(db_txn_adapter3.Get(key1, txn3_got_value).code(),
            absl::StatusCode::kOk);
  EXPECT_EQ(db_txn_adapter3.Put(key1, txn3_got_value + 10).code(),
            absl::StatusCode::kOk);

  // LMDB treat both txns to operate on a view when the txn starts.
  // Both Commmits are OK.
  EXPECT_EQ(db_txn_adapter2.Commit().code(), absl::StatusCode::kOk);
  EXPECT_EQ(db_txn_adapter3.Commit().code(), absl::StatusCode::kOk);

  EXPECT_EQ(txn2_got_value, 2);
  EXPECT_EQ(txn3_got_value, 1);

  // Read the current value of key1 and key2
  ASSERT_EQ(db_txn_adapter1.Begin().code(), absl::StatusCode::kOk);
  int64_t key1_val;
  int64_t key2_val;
  ASSERT_EQ(db_txn_adapter1.Get(key1, key1_val).code(), absl::StatusCode::kOk);
  ASSERT_EQ(db_txn_adapter1.Get(key2, key2_val).code(), absl::StatusCode::kOk);
  ASSERT_EQ(db_txn_adapter1.Commit().code(), absl::StatusCode::kOk);
  // Final value depends on the order of commit.
  // Later commited txn will overwrite the related keys.
  EXPECT_EQ(key1_val, 11);
  EXPECT_EQ(key2_val, 7);
}

}  // namespace

}  // namespace db