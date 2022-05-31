#ifndef SRC_COORDINATOR_START_COORDINATOR_SERVER_H_

#define SRC_COORDINATOR_START_COORDINATOR_SERVER_H_

#include <string>

#include "absl/time/time.h"

namespace coordinator {
void RunServer(const std::string& port,
               absl::Duration default_presumed_abort_duration);

}  // namespace coordinator

#endif  // SRC_COORDINATOR_START_COORDINATOR_SERVER_H_