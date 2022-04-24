#include "src/server/greeter_server.h"

#include "absl/strings/str_cat.h"
#include "grpcpp/server_context.h"
#include "src/proto/greeter.grpc.pb.h"

namespace server {

grpc::Status GreeterServer::Greet(grpc::ServerContext * /*context*/,
                                  const greeter::GreetRequest *request,
                                  greeter::GreetResponse *response) {
  response->set_reply(absl::StrCat("Hello, ", request->name()));
  return grpc::Status::OK;
}

}  // namespace server