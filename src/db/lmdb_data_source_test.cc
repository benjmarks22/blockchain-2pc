#include "src/db/lmdb_data_source.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace db {

namespace {

TEST(LMDBDataSourceTest, ConnectSuccessValidPath) {
  EXPECT_NO_THROW(LMDBDataSource db_source("/tmp"));
}

TEST(LMDBDataSourceTest, ConnectFailWithInvalidPath) {
  EXPECT_THROW(LMDBDataSource db_source("invalid db path"),
               lmdb::runtime_error);
}

TEST(LMDBDataSourceTest, TransactionOps) {
  LMDBDataSource db_source("/tmp");

  // Test: Begin then Commit both succeed.
  absl::Status status = db_source.Begin();
  EXPECT_EQ(status.code(), absl::StatusCode::kOk) << status.ToString();

  status = db_source.Commit();
  EXPECT_EQ(status.code(), absl::StatusCode::kOk) << status.ToString();

  // Test: Commit or Abort after a Commit returns FailedPrecondition error.
  status = db_source.Commit();
  EXPECT_EQ(status.code(), absl::StatusCode::kFailedPrecondition)
      << status.ToString();

  status = db_source.Abort();
  EXPECT_EQ(status.code(), absl::StatusCode::kFailedPrecondition)
      << status.ToString();
}

TEST(LMDBDataSourceTest, SingleTxnPutThenGet) {
  LMDBDataSource db_source("/tmp");

  const std::string key = "key";
  const int64_t put_value = 123;

  // Test: Put value succeed.
  absl::Status status = db_source.Begin();
  ASSERT_EQ(status.code(), absl::StatusCode::kOk) << status.ToString();

  EXPECT_EQ(db_source.Put(key, put_value).code(), absl::StatusCode::kOk);

  status = db_source.Commit();
  EXPECT_EQ(status.code(), absl::StatusCode::kOk) << status.ToString();

  // Test: Get value succeed.
  status = db_source.Begin();
  ASSERT_EQ(status.code(), absl::StatusCode::kOk) << status.ToString();

  int64_t got_value;
  EXPECT_EQ(db_source.Get(key, got_value).code(), absl::StatusCode::kOk);

  status = db_source.Commit();
  EXPECT_EQ(status.code(), absl::StatusCode::kOk) << status.ToString();
  EXPECT_EQ(got_value, put_value);
}

TEST(LMDBDataSourceTest, ParallelTxnsPutAndGet) {
  LMDBDataSource db_source1("/tmp");
  LMDBDataSource db_source2("/tmp");

  ASSERT_EQ(db_source1.Begin().code(), absl::StatusCode::kOk);
  ASSERT_EQ(db_source2.Begin().code(), absl::StatusCode::kOk);
  const std::string key = "key";
  const int64_t put_value = 123;

  // Put value first.
  EXPECT_EQ(db_source1.Put(key, put_value).code(), absl::StatusCode::kOk);

  // Get value when the key not exist in db. Expect internal error.
  int64_t got_value;
  EXPECT_EQ(db_source2.Get(key, got_value).code(), absl::StatusCode::kInternal);
  ASSERT_EQ(db_source2.Abort().code(), absl::StatusCode::kOk);

  // Put txn commited.
  ASSERT_EQ(db_source1.Commit().code(), absl::StatusCode::kOk);

  // Begin Get transaction for 2nd time.
  // Expect Get succeed and got value the same as the one putted.
  ASSERT_EQ(db_source2.Begin().code(), absl::StatusCode::kOk);
  EXPECT_EQ(db_source2.Get(key, got_value).code(), absl::StatusCode::kOk);
  EXPECT_EQ(db_source2.Commit().code(), absl::StatusCode::kOk);
  EXPECT_EQ(got_value, put_value);
}

}  // namespace

}  // namespace db