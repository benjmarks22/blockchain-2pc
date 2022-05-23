#include "src/utils/grpc_utils.h"

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "grpcpp/server_context.h"

namespace utils {

grpc::Status FromAbslStatus(const absl::Status& status,
                            const std::string& prefix) {
  return grpc::Status(static_cast<grpc::StatusCode>(status.raw_code()),
                      absl::StrCat(prefix, std::string(status.message())));
}
}  // namespace utils