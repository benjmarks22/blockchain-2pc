#include "src/client/client.h"

#include <thread>

#include "glog/logging.h"
#include "grpcpp/client_context.h"
#include "src/proto/common.pb.h"

namespace client {

bool Client::TryGetTransactionResults(
    const coordinator::GetTransactionResultRequest& request,
    coordinator::GetTransactionResultResponse& response) {
  grpc::ClientContext response_context;
  const grpc::Status get_status =
      stub_->GetTransactionResult(&response_context, request, &response);
  if (!get_status.ok()) {
    LOG(INFO) << "Error from coordinator " << get_status.error_code() << " "
              << get_status.error_message();
    return false;
  }
  if (response.has_pending_response()) {
    LOG(INFO) << "Still pending";
    return false;
  }
  if (response.has_committed_response() &&
      !response.committed_response().complete()) {
    LOG(INFO) << "Still awaiting some results " << response.DebugString();
    return false;
  }
  LOG(INFO) << "Got final response";
  return true;
}

ResponseOrStatus Client::GetTransactionResults(
    const coordinator::CommitAtomicTransactionResponse& prepare_response) {
  const absl::Time presumed_abort_time =
      absl::FromUnixSeconds(
          prepare_response.config().presumed_abort_time().seconds()) +
      absl::Nanoseconds(
          prepare_response.config().presumed_abort_time().nanos());
  coordinator::GetTransactionResultRequest get_request;
  get_request.set_global_transaction_id(
      prepare_response.global_transaction_id());
  std::this_thread::sleep_until(absl::ToChronoTime(presumed_abort_time));
  while (true) {
    coordinator::GetTransactionResultResponse response;
    if (TryGetTransactionResults(get_request, response)) {
      return response;
    }
    absl::Time now = absl::Now();
    std::this_thread::sleep_until(
        absl::ToChronoTime(now + (now - presumed_abort_time)));
  }
  return grpc::Status::OK;
}

void Client::WaitForCoordinator() {
  coordinator::CommitAtomicTransactionRequest request;
  request.set_client_transaction_id("id");
  while (true) {
    grpc::ClientContext prepare_context;
    coordinator::CommitAtomicTransactionResponse prepare_response;
    const grpc::Status prepare_status = stub_->CommitAtomicTransaction(
        &prepare_context, request, &prepare_response);
    if (prepare_status.error_code() != grpc::UNAVAILABLE) {
      LOG(INFO) << "Coordinator is now ready";
      return;
    }
    LOG(INFO) << "Coordinator unavailable. Waiting another second.";
    std::this_thread::sleep_for(absl::ToChronoSeconds(absl::Seconds(1)));
  }
}

std::future<ResponseOrStatus> Client::CommitAsync(
    const coordinator::CommitAtomicTransactionRequest& request) {
  grpc::ClientContext prepare_context;
  coordinator::CommitAtomicTransactionResponse prepare_response;
  const grpc::Status prepare_status = stub_->CommitAtomicTransaction(
      &prepare_context, request, &prepare_response);
  if (!prepare_status.ok()) {
    std::promise<
        absl::variant<grpc::Status, coordinator::GetTransactionResultResponse>>
        promise;
    promise.set_value(prepare_status);
    return promise.get_future();
  }
  return thread_pool_.submit([this, prepare_response]() {
    return GetTransactionResults(prepare_response);
  });
}

ResponseOrStatus Client::CommitSync(
    const coordinator::CommitAtomicTransactionRequest& request) {
  std::future<ResponseOrStatus> future_response = CommitAsync(request);
  return future_response.get();
}

}  // namespace client