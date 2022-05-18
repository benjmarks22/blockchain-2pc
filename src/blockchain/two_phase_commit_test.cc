#include "src/blockchain/two_phase_commit.h"

#include "absl/status/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {
using ::blockchain::TwoPhaseCommit;

TEST(TwoPhaseCommitTest, UnimplementedErrorCheck) {
  TwoPhaseCommit two_phase_commit;
  EXPECT_THAT(two_phase_commit.StartVoting("t1", 0, 10),
              absl::UnimplementedError("Unimplemented."));
  EXPECT_THAT(two_phase_commit.Vote("t1", 1, TwoPhaseCommit::Ballot::COMMIT),
              absl::UnimplementedError("Unimplemented."));
  EXPECT_THAT(two_phase_commit.GetVotingDecision("t1"),
              absl::UnimplementedError("Unimplemented."));
}

}  // namespace
