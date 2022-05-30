#include "src/coordinator/start_coordinator_server.h"

#include <iostream>
#include <memory>

#include "absl/strings/str_cat.h"
#include "grpc/grpc.h"
#include "grpcpp/security/server_credentials.h"
#include "grpcpp/server.h"
#include "grpcpp/server_builder.h"
#include "src/coordinator/coordinator_server.h"

namespace coordinator {

void RunServer(const std::string& port,
               absl::Duration default_presumed_abort_duration) {
  std::string server_address = absl::StrCat("0.0.0.0:", port);

  CoordinatorServer service(default_presumed_abort_duration);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}
}  // namespace coordinator