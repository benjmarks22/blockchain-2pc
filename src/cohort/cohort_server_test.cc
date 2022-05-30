#include "src/cohort/cohort_server.h"

#include "absl/time/time.h"
#include "gmock/gmock.h"
#include "grpcpp/server_context.h"
#include "gtest/gtest.h"
#include "protobuf-matchers/protocol-buffer-matchers.h"
#include "src/db/database_transaction_adapter.h"
#include "src/proto/cohort.pb.h"
#include "src/proto/common.pb.h"

namespace {
using ::protobuf_matchers::EqualsProto;

class InMemoryDb : public db::DatabaseTransactionAdapter {
 public:
  explicit InMemoryDb(absl::flat_hash_map<std::string, int64_t>& data,
                      absl::Mutex& data_mutex)
      : data_(data), data_mutex_(data_mutex) {}

  [[nodiscard]] bool SupportsConcurrentWrites() const override { return true; }
  absl::Status Begin() override {
    if (in_txn_) {
      return absl::FailedPreconditionError("Already in transaction");
    }
    in_txn_ = true;
    return absl::OkStatus();
  }
  absl::Status BeginReadOnly() override {
    if (in_txn_) {
      return absl::FailedPreconditionError("Already in transaction");
    }
    in_txn_ = true;
    return absl::OkStatus();
  }
  absl::Status Commit() override {
    if (!in_txn_) {
      return absl::FailedPreconditionError("Not in transaction");
    }
    data_mutex_.WriterLock();
    data_.insert(txn_data_.begin(), txn_data_.end());
    data_mutex_.WriterUnlock();
    txn_data_.clear();
    in_txn_ = false;
    return absl::OkStatus();
  }
  absl::Status Abort() override {
    if (!in_txn_) {
      return absl::FailedPreconditionError("Not in transaction");
    }
    txn_data_.clear();
    in_txn_ = false;
    return absl::OkStatus();
  }
  absl::Status Get(const std::string& key, int64_t& output_value) override {
    if (!in_txn_) {
      return absl::FailedPreconditionError("Not in transaction");
    }
    auto value = txn_data_.find(key);
    if (value != txn_data_.end()) {
      output_value = value->second;
    } else {
      data_mutex_.ReaderLock();
      value = data_.find(key);
      if (value == data_.end()) {
        data_mutex_.ReaderUnlock();
        return absl::NotFoundError(
            absl::StrCat("Key could not be found: ", key));
      }
      output_value = value->second;
      data_mutex_.ReaderUnlock();
    }
    return absl::OkStatus();
  }
  absl::Status Put(const std::string& key, int64_t value) override {
    if (!in_txn_) {
      return absl::FailedPreconditionError("Not in transaction");
    }
    txn_data_[key] = value;
    return absl::OkStatus();
  }

 private:
  void Connect() override {}
  bool in_txn_;
  absl::flat_hash_map<std::string, int64_t>& data_;
  absl::flat_hash_map<std::string, int64_t> txn_data_;
  // Necessary since flat_hash_map doesn't support concurrent access.
  absl::Mutex& data_mutex_;
};

std::function<std::unique_ptr<db::DatabaseTransactionAdapter>()>
GetDbCreatorFunc(absl::flat_hash_map<std::string, int64_t>& data,
                 absl::Mutex& data_mutex) {
  return [&data, &data_mutex]() {
    return std::make_unique<InMemoryDb>(data, data_mutex);
  };
}

// TODO(benjmarks22): Add more tests.

TEST(CohortServerTest, GetRequestForNotFoundAborts) {
  grpc::ServerContext context;
  cohort::PrepareTransactionRequest prepare_request;
  cohort::PrepareTransactionResponse prepare_response;
  prepare_request.mutable_config()->mutable_presumed_abort_time()->set_seconds(
      absl::ToUnixSeconds(absl::Now() + absl::Seconds(5)));
  prepare_request.set_transaction_id("id");
  common::Operation* operation =
      prepare_request.mutable_transaction()->add_ops();
  operation->mutable_namespace_()->set_identifier("foo");
  operation->mutable_get()->set_key("a");
  absl::Mutex data_mutex;
  absl::flat_hash_map<std::string, int64_t> data;
  cohort::CohortServer server(1, "/tmp/txn_responses",
                              GetDbCreatorFunc(data, data_mutex));
  EXPECT_TRUE(
      server.PrepareTransaction(&context, &prepare_request, &prepare_response)
          .ok());
  cohort::GetTransactionResultRequest get_request;
  get_request.set_transaction_id("id");
  cohort::GetTransactionResultResponse get_response;
  // Necessary so it can process the transaction.
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_TRUE(
      server.GetTransactionResult(&context, &get_request, &get_response).ok());
  EXPECT_THAT(
      get_response,
      EqualsProto(R"pb(aborted_response:
                           ABORT_REASON_OPERATION_FOR_NON_EXISTENT_VALUE)pb"));
}

}  // namespace