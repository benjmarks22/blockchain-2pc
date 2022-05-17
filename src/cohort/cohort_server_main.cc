#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_cat.h"
#include "grpc/grpc.h"
#include "grpcpp/security/server_credentials.h"
#include "grpcpp/server.h"
#include "grpcpp/server_builder.h"
#include "src/cohort/cohort_server.h"

ABSL_FLAG(std::string, port, "50051", "Port to listen to connections on");
ABSL_FLAG(double, db_thread_ratio, 0.75,
          "Fraction of threads to assign to DB processing. The remaining "
          "threads are used by gRPC.");

void RunServer(const std::string& port, uint num_db_threads) {
  std::string server_address = absl::StrCat("0.0.0.0:", port);
  cohort::CohortServer service(num_db_threads);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  RunServer(absl::GetFlag(FLAGS_port),
            uint(absl::GetFlag(FLAGS_db_thread_ratio) *
                 std::thread::hardware_concurrency()));

  return 0;
}