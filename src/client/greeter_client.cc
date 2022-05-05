#include "src/client/greeter_client.h"

#include "grpcpp/client_context.h"
#include "src/proto/greeter.grpc.pb.h"

namespace client {

void Client::Greet() {
  greeter::GreetRequest request;
  greeter::GreetResponse response;
  grpc::ClientContext context;
  request.set_name("Test");
  grpc::Status status = stub_->Greet(&context, request, &response);
  if (status.ok()) {
    std::cout << response.reply() << std::endl;
  } else {
    std::cout << "Greet rpc failed " << status.error_message() << std::endl;
  }
}

}  // namespace client