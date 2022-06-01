#ifndef SRC_CLIENT_CLIENT_H_

#define SRC_CLIENT_CLIENT_H_

#include <memory>

#include "absl/types/variant.h"
#include "grpcpp/channel.h"
#include "grpcpp/client_context.h"
#include "src/proto/coordinator.grpc.pb.h"
#include "thread_pool.hpp"

namespace client {

using ResponseOrStatus =
    absl::variant<grpc::Status, coordinator::GetTransactionResultResponse>;

class Client {
 public:
  explicit Client(std::shared_ptr<grpc::Channel> channel)
      : stub_(coordinator::Coordinator::NewStub(channel)) {}

  std::future<ResponseOrStatus> CommitAsync(
      const coordinator::CommitAtomicTransactionRequest& request);

  ResponseOrStatus CommitSync(
      const coordinator::CommitAtomicTransactionRequest& request);

  void WaitForCoordinator();

 private:
  bool TryGetTransactionResults(
      const coordinator::GetTransactionResultRequest& request,
      coordinator::GetTransactionResultResponse& response);
  ResponseOrStatus GetTransactionResults(
      const coordinator::CommitAtomicTransactionResponse& prepare_response);

  std::unique_ptr<coordinator::Coordinator::Stub> stub_;
  thread_pool thread_pool_;
};

}  // namespace client

#endif  // SRC_CLIENT_CLIENT_H_