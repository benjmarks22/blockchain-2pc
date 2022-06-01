#include "src/blockchain/two_phase_commit.h"

#include "absl/status/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/blockchain/proto/two_phase_commit_adapter_mock.grpc.pb.h"

namespace {
using ::blockchain::MockTwoPhaseCommitAdapterStub;
using ::blockchain::TwoPhaseCommit;

TEST(TwoPhaseCommitTest, Valid) {
  TwoPhaseCommit two_phase_commit(
      std::make_unique<MockTwoPhaseCommitAdapterStub>());
}

}  // namespace
