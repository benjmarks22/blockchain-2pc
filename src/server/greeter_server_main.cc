#include <iostream>
#include <memory>
#include <string>

#include "grpc/grpc.h"
#include "grpcpp/security/server_credentials.h"
#include "grpcpp/server.h"
#include "grpcpp/server_builder.h"
#include "src/server/greeter_server.h"

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  server::GreeterServer service;

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  std::cout << __cplusplus << std::endl;
  server->Wait();
}

int main(int /*argc*/, char** /*argv*/) {
  RunServer();

  return 0;
}