#ifndef SRC_BLOCKCHAIN_TWO_PHASE_COMMIT_H_

#define SRC_BLOCKCHAIN_TWO_PHASE_COMMIT_H_

#include <ctime>
#include <memory>
#include <string>
#include <thread>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "glog/logging.h"
#include "grpcpp/channel.h"
#include "grpcpp/client_context.h"
#include "src/blockchain/proto/two_phase_commit_adapter.grpc.pb.h"

namespace blockchain {
class TwoPhaseCommit {
 public:
  explicit TwoPhaseCommit(std::shared_ptr<grpc::Channel> channel)
      : TwoPhaseCommit(TwoPhaseCommitAdapter::NewStub(channel)) {
    while (!GetHeartBeat().ok()) {
      LOG(INFO) << "Failed to get heartbeat. Waiting another second.";
      std::this_thread::sleep_for(absl::ToChronoSeconds(absl::Seconds(1)));
    }
  }

  explicit TwoPhaseCommit(
      std::unique_ptr<TwoPhaseCommitAdapter::StubInterface> stub)
      : stub_(stub.release()) {}

  absl::Status StartVoting(const std::string &transaction_id,
                           const std::time_t &timeout_time, int n_participants);

  absl::Status Vote(const std::string &transaction_id, int participant_id,
                    Ballot ballot);

  // Gets the voting decision of a transaction.
  // TODO(heronyang): Attach ABORT_REASON, PENDING_REASON to the response.
  absl::StatusOr<VotingDecision> GetVotingDecision(
      const std::string &transaction_id);

  absl::Status GetHeartBeat();

 private:
  std::unique_ptr<TwoPhaseCommitAdapter::StubInterface> stub_;
};

}  // namespace blockchain

#endif
