#include "src/blockchain/two_phase_commit.h"

namespace blockchain {

absl::Status TwoPhaseCommit::StartVoting(const std::string& /*transaction_id*/,
                                         const std::time_t& /*timeout_time*/,
                                         int /*n_participants*/) {
  // TODO(heronyang): Implement this.
  return absl::UnimplementedError("Unimplemented.");
}

absl::Status TwoPhaseCommit::Vote(const std::string& /*transaction_id*/,
                                  int /*participant_id*/,
                                  TwoPhaseCommit::Ballot /*ballot*/) {
  // TODO(heronyang): Implement this.
  return absl::UnimplementedError("Unimplemented.");
}

absl::StatusOr<TwoPhaseCommit::VotingDecision>
TwoPhaseCommit::GetVotingDecision(const std::string& /*transaction_id*/) {
  // TODO(heronyang): Implement this.
  return absl::UnimplementedError("Unimplemented.");
}

}  // namespace blockchain
