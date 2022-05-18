#ifndef SRC_BLOCKCHAIN_TWO_PHASE_COMMIT_H_

#define SRC_BLOCKCHAIN_TWO_PHASE_COMMIT_H_

#include <ctime>
#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace blockchain {
class TwoPhaseCommit {
 public:
  static absl::Status StartVoting(const std::string &transaction_id,
                                  const std::time_t &timeout_time,
                                  int n_participants);

  enum class Ballot { ABORT, COMMIT };
  static absl::Status Vote(const std::string &transaction_id,
                           int participant_id, Ballot ballot);

  enum class VotingDecision { PENDING, ABORT, COMMIT };
  // Gets the voting decision of a transaction.
  // TODO(heronyang): Attach ABORT_REASON, PENDING_REASON to the response.
  static absl::StatusOr<VotingDecision> GetVotingDecision(
      const std::string &transaction_id);
};

}  // namespace blockchain

#endif
