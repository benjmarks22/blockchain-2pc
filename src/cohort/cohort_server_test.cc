#include "src/cohort/cohort_server.h"

#include "gmock/gmock.h"
#include "grpcpp/server_context.h"
#include "gtest/gtest.h"
#include "protobuf-matchers/protocol-buffer-matchers.h"
#include "src/proto/cohort.pb.h"
#include "src/proto/common.pb.h"

namespace {
using ::protobuf_matchers::EqualsProto;

TEST(CohortServerTest, PrepareTransactionRequestSetsPendingTransaction) {
  grpc::ServerContext context;
  cohort::PrepareTransactionRequest prepare_request;
  cohort::PrepareTransactionResponse prepare_response;
  prepare_request.mutable_config()->mutable_presumed_abort_time()->set_seconds(
      1);
  prepare_request.set_transaction_id("id");
  common::Operation* operation =
      prepare_request.mutable_transaction()->add_ops();
  operation->mutable_namespace_()->set_identifier("foo");
  operation->mutable_get()->set_key("a");
  cohort::CohortServer server;
  EXPECT_TRUE(
      server.PrepareTransaction(&context, &prepare_request, &prepare_response)
          .ok());
  cohort::GetTransactionResultRequest get_request;
  get_request.set_transaction_id("id");
  cohort::GetTransactionResultResponse get_response;
  EXPECT_TRUE(
      server.GetTransactionResult(&context, &get_request, &get_response).ok());
  EXPECT_THAT(get_response, EqualsProto(R"pb(pending_response {})pb"));
}

}  // namespace