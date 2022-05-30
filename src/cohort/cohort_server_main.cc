#include <string>
#include <thread>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "src/cohort/start_cohort_server.h"

ABSL_FLAG(std::string, port, "50051", "Port to listen to connections on");
ABSL_FLAG(double, db_thread_ratio, 0.75,
          "Fraction of threads to assign to DB processing. The remaining "
          "threads are used by gRPC.");
ABSL_FLAG(std::string, db_data_dir, "/tmp/data", "Directory for db data");
ABSL_FLAG(std::string, db_txn_response_dir, "/tmp/txn_responses",
          "Directory for persisted transaction responses in case of a crash "
          "while waiting for the blockchain decision");

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  cohort::RunServer(absl::GetFlag(FLAGS_port),
                    uint(absl::GetFlag(FLAGS_db_thread_ratio) *
                         std::thread::hardware_concurrency()),
                    absl::GetFlag(FLAGS_db_data_dir),
                    absl::GetFlag(FLAGS_db_txn_response_dir));

  return 0;
}