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
#include "src/db/lmdb_database_transaction_adapter.h"

ABSL_FLAG(std::string, port, "50051", "Port to listen to connections on");
ABSL_FLAG(double, db_thread_ratio, 0.75,
          "Fraction of threads to assign to DB processing. The remaining "
          "threads are used by gRPC.");
ABSL_FLAG(std::string, db_data_dir, "/tmp/data", "Directory for db data");
ABSL_FLAG(std::string, db_txn_response_dir, "/tmp/txn_responses",
          "Directory for persisted transaction responses in case of a crash "
          "while waiting for the blockchain decision");

void RunServer(const std::string& port, uint num_db_threads,
               const std::string& db_data_dir,
               const std::string& db_txn_response_dir) {
  std::string server_address = absl::StrCat("0.0.0.0:", port);
  cohort::CohortServer service(
      num_db_threads, db_txn_response_dir, [&db_data_dir]() {
        return std::make_unique<db::LMDBDatabaseTransactionAdapter>(
            db_data_dir);
      });

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
                 std::thread::hardware_concurrency()),
            absl::GetFlag(FLAGS_db_data_dir),
            absl::GetFlag(FLAGS_db_txn_response_dir));

  return 0;
}