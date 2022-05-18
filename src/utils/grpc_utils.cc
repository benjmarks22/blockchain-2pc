#include "src/utils/grpc_utils.h"

#include "absl/status/statusor.h"
#include "grpcpp/server_context.h"

namespace utils {

grpc::Status FromAbslStatus(const absl::Status& status) {
  return grpc::Status(static_cast<grpc::StatusCode>(status.raw_code()),
                      std::string(status.message()));
}
}  // namespace utils