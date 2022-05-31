#include "src/blockchain/two_phase_commit.h"

#include "src/utils/status_utils.h"

namespace blockchain {

absl::Status TwoPhaseCommit::StartVoting(const std::string& transaction_id,
                                         const std::time_t& timeout_time,
                                         int n_participants) {
  grpc::ClientContext context;
  StartVotingRequest request;
  request.set_transaction_id(transaction_id);
  const absl::Time absl_time = absl::FromTimeT(timeout_time);
  timespec ts = absl::ToTimespec(absl_time);
  google::protobuf::Timestamp timestamp;
  request.mutable_timeout_time()->set_seconds(ts.tv_sec);
  request.mutable_timeout_time()->set_nanos(ts.tv_nsec);
  request.set_cohorts(n_participants);
  StartVotingResponse response;
  grpc::Status status = stub_->StartVoting(&context, request, &response);
  return utils::FromGrpcStatus(status, "Failed to start voting");
}

absl::Status TwoPhaseCommit::Vote(const std::string& transaction_id,
                                  int participant_id, Ballot ballot) {
  grpc::ClientContext context;
  VoteRequest request;
  request.set_transaction_id(transaction_id);
  request.set_cohort_id(participant_id);
  request.set_ballot(ballot);
  VoteResponse response;
  grpc::Status status = stub_->Vote(&context, request, &response);
  return utils::FromGrpcStatus(status, "Failed to vote");
}

absl::StatusOr<VotingDecision> TwoPhaseCommit::GetVotingDecision(
    const std::string& transaction_id) {
  grpc::ClientContext context;
  GetVotingDecisionRequest request;
  request.set_transaction_id(transaction_id);
  GetVotingDecisionResponse response;
  grpc::Status status = stub_->GetVotingDecision(&context, request, &response);
  if (!status.ok()) {
    return utils::FromGrpcStatus(status, "Failed to vote");
  }
  return response.decision();
}

}  // namespace blockchain
