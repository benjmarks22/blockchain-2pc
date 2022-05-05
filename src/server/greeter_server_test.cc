#include "src/server/greeter_server.h"

#include "gmock/gmock.h"
#include "grpcpp/server_context.h"
#include "gtest/gtest.h"

namespace {
TEST(GreeterServerTest, ResponseIsCorrect) {
  grpc::ServerContext context;
  greeter::GreetRequest request;
  greeter::GreetResponse response;
  request.set_name("Test");
  EXPECT_TRUE(
      server::GreeterServer().Greet(&context, &request, &response).ok());
  EXPECT_EQ(response.reply(), "Hello, Test");
}

}  // namespace