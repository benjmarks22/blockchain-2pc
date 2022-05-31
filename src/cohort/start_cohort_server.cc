#include "src/cohort/start_cohort_server.h"

#include <filesystem>
#include <iostream>
#include <memory>

#include "absl/strings/str_cat.h"
#include "grpc/grpc.h"
#include "grpcpp/security/server_credentials.h"
#include "grpcpp/server.h"
#include "grpcpp/server_builder.h"
#include "src/cohort/cohort_server.h"
#include "src/db/lmdb_database_transaction_adapter.h"

namespace cohort {

void RunServer(const std::string& port, uint num_db_threads,
               const std::string& db_data_dir,
               const std::string& db_txn_response_dir) {
  std::filesystem::create_directories(db_data_dir);
  std::filesystem::create_directories(db_txn_response_dir);
  std::string server_address = absl::StrCat("0.0.0.0:", port);
  CohortServer service(num_db_threads, db_txn_response_dir, [&db_data_dir]() {
    return std::make_unique<db::LMDBDatabaseTransactionAdapter>(db_data_dir);
  });

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}
}  // namespace cohort