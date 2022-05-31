#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/time/time.h"
#include "src/coordinator/start_coordinator_server.h"

ABSL_FLAG(std::string, port, "50052", "Port to listen to connections on");
ABSL_FLAG(
    std::string, default_presumed_abort_duration, "1m",
    "Default duration for the presumed abort time relative to the current "
    "time. Only used if the client does not specify the timestamp.");

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  absl::Duration duration;
  std::string error;
  if (absl::ParseFlag(absl::GetFlag(FLAGS_default_presumed_abort_duration),
                      &duration, &error)) {
    coordinator::RunServer(absl::GetFlag(FLAGS_port), duration);
  } else {
    std::fprintf(
        stderr,
        "Error parsing default duration for the presumed abort time: %s.\n",
        error.c_str());
    return 1;
  }

  return 0;
}