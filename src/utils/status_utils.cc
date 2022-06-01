#include "src/utils/status_utils.h"

#include "absl/strings/str_cat.h"
#include "grpcpp/server_context.h"

namespace utils {

grpc::Status FromAbslStatus(const absl::Status& status,
                            const std::string& prefix) {
  return grpc::Status(static_cast<grpc::StatusCode>(status.raw_code()),
                      absl::StrCat(prefix, " ", std::string(status.message())));
}
absl::Status FromGrpcStatus(const grpc::Status& status,
                            const std::string& prefix) {
  return absl::Status(
      static_cast<absl::StatusCode>(status.error_code()),
      absl::StrCat(prefix, std::string(status.error_message())));
}
}  // namespace utils