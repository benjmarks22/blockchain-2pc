#include "src/coordinator/coordinator_server.h"

#include "absl/time/time.h"
#include "gmock/gmock.h"
#include "grpcpp/server_context.h"
#include "gtest/gtest.h"
#include "protobuf-matchers/protocol-buffer-matchers.h"
#include "src/blockchain/proto/two_phase_commit_adapter_mock.grpc.pb.h"
#include "src/proto/cohort_mock.grpc.pb.h"
#include "src/proto/common.pb.h"
#include "src/proto/coordinator.pb.h"

namespace {
using ::protobuf_matchers::EquivToProto;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::Return;
using ::testing::SetArgReferee;

class CoordinatorWithMockCohorts : public coordinator::CoordinatorServer {
 public:
  explicit CoordinatorWithMockCohorts(
      absl::Duration default_presumed_abort_duration)
      : coordinator::CoordinatorServer(
            default_presumed_abort_duration,
            std::make_unique<blockchain::TwoPhaseCommit>(
                std::make_unique<
                    blockchain::MockTwoPhaseCommitAdapterStub>())) {}

  MOCK_METHOD4(MockPrepareCohortTransaction,
               void(const std::string& transaction_id,
                    const common::Namespace& namespace_,
                    const cohort::PrepareTransactionRequest& request,
                    const grpc::ServerContext& context));
  MOCK_METHOD4(MockGetResultsFromCohort,
               grpc::Status(const common::Namespace& namespace_,
                            const cohort::GetTransactionResultRequest& request,
                            grpc::ClientContext& context,
                            cohort::GetTransactionResultResponse& response));
  MOCK_METHOD3(
      MockStartVoting,
      absl::Status(const std::string& transaction_id,
                   const google::protobuf::Timestamp& presumed_abort_time,
                   size_t num_cohorts));
  MOCK_METHOD1(MockGetVotingDecision,
               absl::StatusOr<blockchain::VotingDecision>(
                   const std::string& transaction_id));
  MOCK_METHOD0(MockNow, absl::Time());

 private:
  bool SortCohortRequests() override { return true; }
  absl::Time Now() override { return MockNow(); }
  void PrepareCohortTransaction(
      const std::string& transaction_id, const common::Namespace& namespace_,
      const cohort::PrepareTransactionRequest& request,
      const grpc::ServerContext& context) override {
    MockPrepareCohortTransaction(transaction_id, namespace_, request, context);
  }
  grpc::Status GetResultsFromCohort(
      const common::Namespace& namespace_,
      const cohort::GetTransactionResultRequest& request,
      grpc::ClientContext& context,
      cohort::GetTransactionResultResponse& response) override {
    return MockGetResultsFromCohort(namespace_, request, context, response);
  }
  absl::Status StartVoting(
      const std::string& transaction_id,
      const google::protobuf::Timestamp& presumed_abort_time,
      size_t num_cohorts) override {
    return MockStartVoting(transaction_id, presumed_abort_time, num_cohorts);
  }
  absl::StatusOr<blockchain::VotingDecision> GetVotingDecision(
      const std::string& transaction_id) override {
    return MockGetVotingDecision(transaction_id);
  }
};

google::protobuf::Timestamp FromAbslTime(absl::Time time) {
  timespec ts = absl::ToTimespec(time);
  google::protobuf::Timestamp timestamp;
  timestamp.set_seconds(ts.tv_sec);
  timestamp.set_nanos(ts.tv_nsec);
  return timestamp;
}

void EXPECT_OK(const grpc::Status& status) {
  EXPECT_EQ(status.error_code(), grpc::OK) << status.error_message();
}

void ExpectTimestamp(const google::protobuf::Timestamp& actual_timestamp,
                     absl::Time expected_time) {
  EXPECT_THAT(actual_timestamp, EquivToProto(FromAbslTime(expected_time)));
}

common::Transaction TwoNamespaceReadWriteTransaction() {
  common::Transaction transaction;
  common::Operation* operation = transaction.add_ops();
  operation->mutable_namespace_()->set_address("namespace1");
  operation->mutable_get()->set_key("a");
  common::Operation* operation2 = transaction.add_ops();
  operation2->mutable_namespace_()->set_address("namespace2");
  operation2->mutable_put()->set_key("b");
  operation2->mutable_put()
      ->mutable_value()
      ->mutable_constant_value()
      ->set_int64_value(1);
  return transaction;
}

common::Transaction SingleNamespaceReadOnlyTransaction() {
  common::Transaction transaction;
  common::Operation* operation = transaction.add_ops();
  operation->mutable_namespace_()->set_address("fake_address");
  operation->mutable_get()->set_key("a");
  return transaction;
}

common::Transaction SingleNamespaceWriteOnlyTransaction() {
  common::Transaction transaction;
  common::Operation* operation = transaction.add_ops();
  operation->mutable_namespace_()->set_address("fake_address");
  operation->mutable_put()->set_key("b");
  operation->mutable_put()
      ->mutable_value()
      ->mutable_constant_value()
      ->set_int64_value(1);
  return transaction;
}

TEST(CoordinatorServerTest, CommitHasDeterministicTransactionId) {
  // For the same client transaction id, the output transaction id should be the
  // same. Different client transaction ids should be different.
  absl::Time start_time = absl::FromUnixSeconds(10);
  grpc::ServerContext context;
  coordinator::CommitAtomicTransactionRequest commit_request;
  coordinator::CommitAtomicTransactionResponse commit_response;
  commit_request.set_client_transaction_id("id");
  *commit_request.mutable_transaction() = SingleNamespaceReadOnlyTransaction();

  CoordinatorWithMockCohorts server(absl::Minutes(1));
  EXPECT_CALL(server, MockNow()).WillRepeatedly(Return(start_time));
  EXPECT_CALL(server, MockPrepareCohortTransaction(_, _, _, _));
  EXPECT_OK(server.CommitAtomicTransaction(&context, &commit_request,
                                           &commit_response));
  EXPECT_THAT(commit_response.global_transaction_id(), Not(IsEmpty()));
  const std::string transaction_id = commit_response.global_transaction_id();

  CoordinatorWithMockCohorts server2(absl::Minutes(1));
  EXPECT_CALL(server2, MockNow()).WillRepeatedly(Return(start_time));
  EXPECT_CALL(server2, MockPrepareCohortTransaction(_, _, _, _));
  EXPECT_OK(server2.CommitAtomicTransaction(&context, &commit_request,
                                            &commit_response));
  EXPECT_EQ(commit_response.global_transaction_id(), transaction_id);

  CoordinatorWithMockCohorts server3(absl::Minutes(1));
  EXPECT_CALL(server3, MockNow()).WillRepeatedly(Return(start_time));
  EXPECT_CALL(server3, MockPrepareCohortTransaction(_, _, _, _));
  commit_request.set_client_transaction_id("different id");
  EXPECT_OK(server3.CommitAtomicTransaction(&context, &commit_request,
                                            &commit_response));
  EXPECT_THAT(commit_response.global_transaction_id(), Not(IsEmpty()));
  EXPECT_NE(commit_response.global_transaction_id(), transaction_id);
}

TEST(CoordinatorServerTest, CommitHasCorrectDefaultTimestamp) {
  absl::Time start_time = absl::FromUnixSeconds(10);
  grpc::ServerContext context;
  coordinator::CommitAtomicTransactionRequest commit_request;
  coordinator::CommitAtomicTransactionResponse commit_response;
  commit_request.set_client_transaction_id("id");
  *commit_request.mutable_transaction() = SingleNamespaceReadOnlyTransaction();

  CoordinatorWithMockCohorts server(absl::Minutes(1));
  EXPECT_CALL(server, MockNow()).WillRepeatedly(Return(start_time));
  EXPECT_CALL(
      server,
      MockPrepareCohortTransaction(
          _, _,
          EquivToProto(
              R"pb(transaction {
                     ops {
                       namespace { address: "fake_address" }
                       get { key: "a" }
                     }
                   }
                   config { presumed_abort_time { seconds: 70 nanos: 0 } }
                   transaction_id: "fdd2e7924aa194de6ac1d2744de0454cb53d0b10fd7eb56a50b72cec4fb74949"
                   only_cohort: true
              )pb"),
          _));
  EXPECT_OK(server.CommitAtomicTransaction(&context, &commit_request,
                                           &commit_response));
  ExpectTimestamp(commit_response.config().presumed_abort_time(),
                  start_time + absl::Minutes(1));
}

TEST(CoordinatorServerTest, PastTimestampDoesNotCommit) {
  absl::Time start_time = absl::FromUnixSeconds(10);
  grpc::ServerContext context;
  coordinator::CommitAtomicTransactionRequest commit_request;
  coordinator::CommitAtomicTransactionResponse commit_response;
  commit_request.set_client_transaction_id("id");
  *commit_request.mutable_config()->mutable_presumed_abort_time() =
      FromAbslTime(start_time - absl::Microseconds(1));
  *commit_request.mutable_transaction() = SingleNamespaceReadOnlyTransaction();

  CoordinatorWithMockCohorts server(absl::Minutes(1));
  EXPECT_CALL(server, MockNow()).WillRepeatedly(Return(start_time));
  EXPECT_EQ(
      server
          .CommitAtomicTransaction(&context, &commit_request, &commit_response)
          .error_code(),
      grpc::INVALID_ARGUMENT);
}

TEST(CoordinatorServerTest, CommitHasCorrectProvidedTimestamp) {
  absl::Time start_time = absl::FromUnixSeconds(10);
  grpc::ServerContext context;
  coordinator::CommitAtomicTransactionRequest commit_request;
  coordinator::CommitAtomicTransactionResponse commit_response;
  commit_request.set_client_transaction_id("id");
  *commit_request.mutable_config()->mutable_presumed_abort_time() =
      FromAbslTime(start_time + absl::Seconds(5));
  *commit_request.mutable_transaction() = SingleNamespaceReadOnlyTransaction();

  CoordinatorWithMockCohorts server(absl::Minutes(1));
  EXPECT_CALL(server, MockNow()).WillRepeatedly(Return(start_time));
  EXPECT_CALL(server, MockPrepareCohortTransaction(_, _, _, _));
  EXPECT_OK(server.CommitAtomicTransaction(&context, &commit_request,
                                           &commit_response));
  ExpectTimestamp(commit_response.config().presumed_abort_time(),
                  start_time + absl::Seconds(5));
}

TEST(CoordinatorServerTest, CommitFailsIfStartVotingFails) {
  absl::Time start_time = absl::FromUnixSeconds(10);
  grpc::ServerContext context;
  coordinator::CommitAtomicTransactionRequest commit_request;
  coordinator::CommitAtomicTransactionResponse commit_response;
  commit_request.set_client_transaction_id("id");
  *commit_request.mutable_transaction() = TwoNamespaceReadWriteTransaction();

  CoordinatorWithMockCohorts server(absl::Minutes(1));
  EXPECT_CALL(server, MockNow()).WillRepeatedly(Return(start_time));
  EXPECT_CALL(
      server,
      MockStartVoting(
          _, EquivToProto(FromAbslTime(start_time + absl::Minutes(1))), Eq(2)))
      .WillOnce(Return(absl::AlreadyExistsError("Failure")));
  EXPECT_EQ(
      server
          .CommitAtomicTransaction(&context, &commit_request, &commit_response)
          .error_code(),
      grpc::ALREADY_EXISTS);
}

TEST(CoordinatorServerTest, CommitStartsVotingForMultiNamespaceTransaction) {
  absl::Time start_time = absl::FromUnixSeconds(10);
  grpc::ServerContext context;
  coordinator::CommitAtomicTransactionRequest commit_request;
  coordinator::CommitAtomicTransactionResponse commit_response;
  commit_request.set_client_transaction_id("id");
  *commit_request.mutable_transaction() = TwoNamespaceReadWriteTransaction();

  CoordinatorWithMockCohorts server(absl::Minutes(1));
  EXPECT_CALL(server, MockNow()).WillRepeatedly(Return(start_time));
  EXPECT_CALL(
      server,
      MockStartVoting(
          _, EquivToProto(FromAbslTime(start_time + absl::Minutes(1))), Eq(2)))
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(
      server,
      MockPrepareCohortTransaction(
          _, EquivToProto(R"pb(address: "namespace1")pb"),
          EquivToProto(
              R"pb(transaction {
                     ops {
                       namespace { address: "namespace1" }
                       get { key: "a" }
                     }
                   }
                   config { presumed_abort_time { seconds: 70 nanos: 0 } }
                   transaction_id: "fdd2e7924aa194de6ac1d2744de0454cb53d0b10fd7eb56a50b72cec4fb74949"
                   cohort_index: 0
              )pb"),
          _));
  EXPECT_CALL(
      server,
      MockPrepareCohortTransaction(
          _, EquivToProto(R"pb(address: "namespace2")pb"),
          EquivToProto(
              R"pb(transaction {
                     ops {
                       namespace { address: "namespace2" }
                       put {
                         key: "b"
                         value { constant_value { int64_value: 1 } }
                       }
                     }
                   }
                   config { presumed_abort_time { seconds: 70 nanos: 0 } }
                   transaction_id: "fdd2e7924aa194de6ac1d2744de0454cb53d0b10fd7eb56a50b72cec4fb74949"
                   cohort_index: 1
              )pb"),
          _));
  EXPECT_OK(server.CommitAtomicTransaction(&context, &commit_request,
                                           &commit_response));
}

TEST(CoordinatorServerTest, NotFoundTransactionDoesNotGetResults) {
  absl::Time start_time = absl::FromUnixSeconds(10);
  grpc::ServerContext context;
  coordinator::CommitAtomicTransactionRequest commit_request;
  coordinator::CommitAtomicTransactionResponse commit_response;
  commit_request.set_client_transaction_id("id");
  *commit_request.mutable_transaction() = SingleNamespaceReadOnlyTransaction();

  CoordinatorWithMockCohorts server(absl::Minutes(1));
  EXPECT_CALL(server, MockNow()).WillRepeatedly(Return(start_time));
  EXPECT_CALL(server, MockPrepareCohortTransaction(_, _, _, _));
  EXPECT_OK(server.CommitAtomicTransaction(&context, &commit_request,
                                           &commit_response));
  coordinator::GetTransactionResultRequest get_request;
  get_request.set_global_transaction_id("Not found");
  coordinator::GetTransactionResultResponse get_response;
  EXPECT_EQ(server.GetTransactionResult(&context, &get_request, &get_response)
                .error_code(),
            grpc::NOT_FOUND);
}

TEST(CoordinatorServerTest, GetsResultsWhenCommitted) {
  absl::Time start_time = absl::FromUnixSeconds(10);
  grpc::ServerContext context;
  coordinator::CommitAtomicTransactionRequest commit_request;
  coordinator::CommitAtomicTransactionResponse commit_response;
  commit_request.set_client_transaction_id("id");
  *commit_request.mutable_transaction() = TwoNamespaceReadWriteTransaction();

  CoordinatorWithMockCohorts server(absl::Minutes(1));
  EXPECT_CALL(server, MockNow()).WillRepeatedly(Return(start_time));
  EXPECT_CALL(server, MockStartVoting(_, _, _))
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(server, MockPrepareCohortTransaction(_, _, _, _)).Times(2);
  EXPECT_OK(server.CommitAtomicTransaction(&context, &commit_request,
                                           &commit_response));
  coordinator::GetTransactionResultRequest get_request;
  get_request.set_global_transaction_id(
      commit_response.global_transaction_id());
  coordinator::GetTransactionResultResponse get_response;
  EXPECT_CALL(server, MockGetVotingDecision(_))
      .WillOnce(Return(blockchain::VotingDecision::VOTING_DECISION_COMMIT));
  cohort::GetTransactionResultResponse mock_cohort_response;
  mock_cohort_response.mutable_committed_response();
  cohort::GetTransactionResultResponse mock_cohort_response2;
  auto* cohort_get_response =
      mock_cohort_response2.mutable_committed_response()->add_get_responses();
  cohort_get_response->mutable_get()->set_key("b");
  cohort_get_response->mutable_namespace_()->set_address("namespace2");
  cohort_get_response->mutable_value()->set_int64_value(3);
  EXPECT_CALL(server, MockGetResultsFromCohort(_, _, _, _))
      .WillOnce(DoAll(SetArgReferee<3>(mock_cohort_response),
                      Return(grpc::Status::OK)))
      .WillOnce(DoAll(SetArgReferee<3>(mock_cohort_response2),
                      Return(grpc::Status::OK)));
  EXPECT_OK(server.GetTransactionResult(&context, &get_request, &get_response));
  EXPECT_THAT(get_response,
              EquivToProto(R"pb(committed_response {
                                  complete: true
                                  response {
                                    get_responses {
                                      namespace { address: "namespace2" }
                                      get { key: "b" }
                                      value { int64_value: 3 }
                                    }
                                  }
                                })pb"));
}

TEST(CoordinatorServerTest, GetsResultsWhenCommittedSingleNamespace) {
  absl::Time start_time = absl::FromUnixSeconds(10);
  grpc::ServerContext context;
  coordinator::CommitAtomicTransactionRequest commit_request;
  coordinator::CommitAtomicTransactionResponse commit_response;
  commit_request.set_client_transaction_id("id");
  *commit_request.mutable_transaction() = SingleNamespaceReadOnlyTransaction();

  CoordinatorWithMockCohorts server(absl::Minutes(1));
  EXPECT_CALL(server, MockNow()).WillRepeatedly(Return(start_time));
  EXPECT_CALL(server, MockPrepareCohortTransaction(_, _, _, _)).Times(1);
  EXPECT_OK(server.CommitAtomicTransaction(&context, &commit_request,
                                           &commit_response));
  coordinator::GetTransactionResultRequest get_request;
  get_request.set_global_transaction_id(
      commit_response.global_transaction_id());
  coordinator::GetTransactionResultResponse get_response;
  cohort::GetTransactionResultResponse mock_cohort_response;
  auto* cohort_get_response =
      mock_cohort_response.mutable_committed_response()->add_get_responses();
  cohort_get_response->mutable_get()->set_key("a");
  cohort_get_response->mutable_namespace_()->set_address("fake_address");
  cohort_get_response->mutable_value()->set_int64_value(3);
  EXPECT_CALL(server, MockGetResultsFromCohort(_, _, _, _))
      .WillOnce(DoAll(SetArgReferee<3>(mock_cohort_response),
                      Return(grpc::Status::OK)));
  EXPECT_OK(server.GetTransactionResult(&context, &get_request, &get_response));
  EXPECT_THAT(get_response,
              EquivToProto(R"pb(committed_response {
                                  complete: true
                                  response {
                                    get_responses {
                                      namespace { address: "fake_address" }
                                      get { key: "a" }
                                      value { int64_value: 3 }
                                    }
                                  }
                                })pb"));
}

TEST(CoordinatorServerTest, GetsResultsWhenCommittedWriteOnly) {
  absl::Time start_time = absl::FromUnixSeconds(10);
  grpc::ServerContext context;
  coordinator::CommitAtomicTransactionRequest commit_request;
  coordinator::CommitAtomicTransactionResponse commit_response;
  commit_request.set_client_transaction_id("id");
  *commit_request.mutable_transaction() = SingleNamespaceWriteOnlyTransaction();

  CoordinatorWithMockCohorts server(absl::Minutes(1));
  EXPECT_CALL(server, MockNow()).WillRepeatedly(Return(start_time));
  EXPECT_CALL(server, MockPrepareCohortTransaction(_, _, _, _)).Times(1);
  EXPECT_OK(server.CommitAtomicTransaction(&context, &commit_request,
                                           &commit_response));
  coordinator::GetTransactionResultRequest get_request;
  get_request.set_global_transaction_id(
      commit_response.global_transaction_id());
  coordinator::GetTransactionResultResponse get_response;
  cohort::GetTransactionResultResponse mock_cohort_response;
  mock_cohort_response.mutable_committed_response();
  EXPECT_CALL(server, MockGetResultsFromCohort(_, _, _, _))
      .WillOnce(DoAll(SetArgReferee<3>(mock_cohort_response),
                      Return(grpc::Status::OK)));
  EXPECT_OK(server.GetTransactionResult(&context, &get_request, &get_response));
  EXPECT_THAT(get_response,
              EquivToProto(R"pb(committed_response { complete: true })pb"));
}

TEST(CoordinatorServerTest, GetsResultsWhenNotFound) {
  absl::Time start_time = absl::FromUnixSeconds(10);
  grpc::ServerContext context;
  coordinator::CommitAtomicTransactionRequest commit_request;
  coordinator::CommitAtomicTransactionResponse commit_response;
  commit_request.set_client_transaction_id("id");
  *commit_request.mutable_transaction() = SingleNamespaceReadOnlyTransaction();

  CoordinatorWithMockCohorts server(absl::Minutes(1));
  EXPECT_CALL(server, MockNow()).WillRepeatedly(Return(start_time));
  EXPECT_CALL(server, MockPrepareCohortTransaction(_, _, _, _)).Times(1);
  EXPECT_OK(server.CommitAtomicTransaction(&context, &commit_request,
                                           &commit_response));
  coordinator::GetTransactionResultRequest get_request;
  get_request.set_global_transaction_id("Not found transaction id");
  coordinator::GetTransactionResultResponse get_response;
  EXPECT_EQ(server.GetTransactionResult(&context, &get_request, &get_response)
                .error_code(),
            grpc::NOT_FOUND);
}

TEST(CoordinatorServerTest, GetsResultsWhenPendingVote) {
  absl::Time start_time = absl::FromUnixSeconds(10);
  grpc::ServerContext context;
  coordinator::CommitAtomicTransactionRequest commit_request;
  coordinator::CommitAtomicTransactionResponse commit_response;
  commit_request.set_client_transaction_id("id");
  *commit_request.mutable_transaction() = TwoNamespaceReadWriteTransaction();

  CoordinatorWithMockCohorts server(absl::Minutes(1));
  EXPECT_CALL(server, MockNow()).WillRepeatedly(Return(start_time));
  EXPECT_CALL(server, MockStartVoting(_, _, _))
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(server, MockPrepareCohortTransaction(_, _, _, _)).Times(2);
  EXPECT_OK(server.CommitAtomicTransaction(&context, &commit_request,
                                           &commit_response));
  coordinator::GetTransactionResultRequest get_request;
  get_request.set_global_transaction_id(
      commit_response.global_transaction_id());
  coordinator::GetTransactionResultResponse get_response;

  EXPECT_CALL(server, MockGetVotingDecision(_))
      .WillOnce(Return(blockchain::VotingDecision::VOTING_DECISION_PENDING));
  EXPECT_OK(server.GetTransactionResult(&context, &get_request, &get_response));
  EXPECT_THAT(get_response, EquivToProto(R"pb(pending_response {})pb"));
}

TEST(CoordinatorServerTest, GetsResultsWhenAbortedSingleNamespace) {
  absl::Time start_time = absl::FromUnixSeconds(10);
  grpc::ServerContext context;
  coordinator::CommitAtomicTransactionRequest commit_request;
  coordinator::CommitAtomicTransactionResponse commit_response;
  commit_request.set_client_transaction_id("id");
  *commit_request.mutable_transaction() = SingleNamespaceReadOnlyTransaction();

  CoordinatorWithMockCohorts server(absl::Minutes(1));
  EXPECT_CALL(server, MockNow()).WillRepeatedly(Return(start_time));
  EXPECT_CALL(server, MockPrepareCohortTransaction(_, _, _, _)).Times(1);
  EXPECT_OK(server.CommitAtomicTransaction(&context, &commit_request,
                                           &commit_response));
  coordinator::GetTransactionResultRequest get_request;
  get_request.set_global_transaction_id(
      commit_response.global_transaction_id());
  coordinator::GetTransactionResultResponse get_response;
  cohort::GetTransactionResultResponse mock_cohort_response;
  mock_cohort_response.set_aborted_response(
      common::ABORT_REASON_RESOURCE_LOCKED);
  EXPECT_CALL(server, MockGetResultsFromCohort(_, _, _, _))
      .WillOnce(DoAll(SetArgReferee<3>(mock_cohort_response),
                      Return(grpc::Status::OK)));
  EXPECT_OK(server.GetTransactionResult(&context, &get_request, &get_response));
  EXPECT_THAT(get_response,
              EquivToProto(R"pb(aborted_response {
                                  namespaces { address: "fake_address" }
                                  reason: ABORT_REASON_RESOURCE_LOCKED
                                })pb"));
}

TEST(CoordinatorServerTest, GetsResultsWhenCommittedPartialResponse) {
  absl::Time start_time = absl::FromUnixSeconds(10);
  grpc::ServerContext context;
  coordinator::CommitAtomicTransactionRequest commit_request;
  coordinator::CommitAtomicTransactionResponse commit_response;
  commit_request.set_client_transaction_id("id");
  *commit_request.mutable_transaction() = TwoNamespaceReadWriteTransaction();

  CoordinatorWithMockCohorts server(absl::Minutes(1));
  EXPECT_CALL(server, MockNow()).WillRepeatedly(Return(start_time));
  EXPECT_CALL(server, MockStartVoting(_, _, _))
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(server, MockPrepareCohortTransaction(_, _, _, _)).Times(2);
  EXPECT_OK(server.CommitAtomicTransaction(&context, &commit_request,
                                           &commit_response));
  coordinator::GetTransactionResultRequest get_request;
  get_request.set_global_transaction_id(
      commit_response.global_transaction_id());
  coordinator::GetTransactionResultResponse get_response;
  EXPECT_CALL(server, MockGetVotingDecision(_))
      .WillOnce(Return(blockchain::VotingDecision::VOTING_DECISION_COMMIT));
  cohort::GetTransactionResultResponse mock_cohort_response;
  mock_cohort_response.mutable_committed_response();
  EXPECT_CALL(server, MockGetResultsFromCohort(_, _, _, _))
      .WillOnce(DoAll(SetArgReferee<3>(mock_cohort_response),
                      Return(grpc::Status::OK)))
      .WillOnce(
          Return(grpc::Status(grpc::DEADLINE_EXCEEDED, "Deadline exceeded")));
  EXPECT_OK(server.GetTransactionResult(&context, &get_request, &get_response));
  EXPECT_THAT(get_response, EquivToProto(R"pb(committed_response {})pb"));
}

TEST(CoordinatorServerTest, GetsResultsWhenAborted) {
  absl::Time start_time = absl::FromUnixSeconds(10);
  grpc::ServerContext context;
  coordinator::CommitAtomicTransactionRequest commit_request;
  coordinator::CommitAtomicTransactionResponse commit_response;
  commit_request.set_client_transaction_id("id");
  *commit_request.mutable_transaction() = TwoNamespaceReadWriteTransaction();

  CoordinatorWithMockCohorts server(absl::Minutes(1));
  EXPECT_CALL(server, MockNow()).WillRepeatedly(Return(start_time));
  EXPECT_CALL(server, MockStartVoting(_, _, _))
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(server, MockPrepareCohortTransaction(_, _, _, _)).Times(2);
  EXPECT_OK(server.CommitAtomicTransaction(&context, &commit_request,
                                           &commit_response));
  coordinator::GetTransactionResultRequest get_request;
  get_request.set_global_transaction_id(
      commit_response.global_transaction_id());
  coordinator::GetTransactionResultResponse get_response;
  EXPECT_CALL(server, MockGetVotingDecision(_))
      .WillOnce(Return(blockchain::VotingDecision::VOTING_DECISION_ABORT));
  EXPECT_OK(server.GetTransactionResult(&context, &get_request, &get_response));
  EXPECT_THAT(get_response, EquivToProto(R"pb(aborted_response {})pb"));
}

}  // namespace