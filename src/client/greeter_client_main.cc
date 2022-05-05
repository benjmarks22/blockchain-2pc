#include <iostream>

#include "grpc/grpc.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"
#include "src/client/greeter_client.h"
#include "src/proto/greeter.grpc.pb.h"

int main(int /*argc*/, char** /*argv*/) {
  client::Client client(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));

  std::cout << "-------------- Greet --------------" << std::endl;
  client.Greet();

  return 0;
}