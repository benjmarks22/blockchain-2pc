#ifndef SRC_COHORT_START_COHORT_SERVER_H_

#define SRC_COHORT_START_COHORT_SERVER_H_

#include <string>

namespace cohort {
void RunServer(const std::string& port, uint num_db_threads,
               const std::string& db_data_dir,
               const std::string& db_txn_response_dir);

}  // namespace cohort

#endif  // SRC_COHORT_START_COHORT_SERVER_H_