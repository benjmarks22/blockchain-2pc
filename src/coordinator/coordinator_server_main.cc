#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_cat.h"
#include "grpc/grpc.h"
#include "grpcpp/security/server_credentials.h"
#include "grpcpp/server.h"
#include "grpcpp/server_builder.h"
#include "src/coordinator/coordinator_server.h"

ABSL_FLAG(std::string, port, "50052", "Port to listen to connections on");
ABSL_FLAG(
    std::string, default_presumed_abort_duration, "1m",
    "Default duration for the presumed abort time relative to the current "
    "time. Only used if the client does not specify the timestamp.");

void RunServer(const std::string& port,
               absl::Duration default_presumed_abort_duration) {
  std::string server_address = absl::StrCat("0.0.0.0:", port);

  coordinator::CoordinatorServer service(default_presumed_abort_duration);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  absl::Duration duration;
  std::string error;
  if (absl::ParseFlag(absl::GetFlag(FLAGS_default_presumed_abort_duration),
                      &duration, &error)) {
    RunServer(absl::GetFlag(FLAGS_port), duration);
  } else {
    std::printf(
        "Error parsing default duration for the presumed abort time: %s.\n",
        error.c_str());
  }

  return 0;
}