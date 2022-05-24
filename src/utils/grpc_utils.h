#ifndef SRC_UTILS_GRPC_UTILS_H_

#define SRC_UTILS_GRPC_UTILS_H_

#include "absl/status/status.h"
#include "grpcpp/server_context.h"

namespace utils {
grpc::Status FromAbslStatus(const absl::Status& status,
                            const std::string& prefix);
}  // namespace utils

#endif  // SRC_UTILS_GRPC_UTILS_H_