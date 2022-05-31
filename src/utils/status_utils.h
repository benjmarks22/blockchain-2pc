#ifndef SRC_UTILS_STATUS_UTILS_H_

#define SRC_UTILS_STATUS_UTILS_H_

#include "absl/status/status.h"
#include "grpcpp/server_context.h"

namespace utils {
grpc::Status FromAbslStatus(const absl::Status& status,
                            const std::string& prefix);
absl::Status FromGrpcStatus(const grpc::Status& status,
                            const std::string& prefix);

#define ASSIGN_OR_RETURN(lhs, rexpr)         \
  auto result_assign_or_return = rexpr;      \
  if (!result_assign_or_return.ok()) {       \
    return result_assign_or_return.status(); \
  }                                          \
  lhs = result_assign_or_return.value();

#define RETURN_IF_ERROR(expr)                 \
  absl::Status return_if_error_status = expr; \
  if (!return_if_error_status.ok()) {         \
    return return_if_error_status;            \
  }
}  // namespace utils

#endif  // SRC_UTILS_STATUS_UTILS_H_