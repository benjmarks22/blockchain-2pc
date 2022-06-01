#ifndef SRC_COORDINATOR_COORDINATOR_SERVER_H_

#define SRC_COORDINATOR_COORDINATOR_SERVER_H_

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "grpcpp/server_context.h"
#include "src/blockchain/two_phase_commit.h"
#include "src/proto/cohort.grpc.pb.h"
#include "src/proto/common.pb.h"
#include "src/proto/coordinator.grpc.pb.h"

namespace coordinator {

namespace internal {

struct TransactionMetadata {
  common::TransactionConfig config;
  std::vector<std::unique_ptr<grpc::ClientContext>> contexts;
  std::vector<common::Namespace> cohort_namespaces;
  // This allows skipping the blockchain since there's no need to agree on the
  // commit decision when only one cohort is involved.
  absl::optional<common::Namespace> single_cohort_namespace;
  // This indicates that the transaction may have been sent to all the cohorts
  // and thus might commit.
  bool possibly_sent_to_all_cohorts;
  blockchain::VotingDecision decision;
  absl::flat_hash_set<std::string> cohorts_already_responded;
  // TODO(benjmarks22): Add locks so that only one thread can access this data
  // at once.
};

struct TransactionResponse {
  bool complete;
  GetTransactionResultResponse response;
};

struct SubTransaction {
  common::Namespace namespace_;
  common::Transaction transaction;
};

}  // namespace internal

class CoordinatorServer : public Coordinator::Service {
 public:
  explicit CoordinatorServer(
      absl::Duration default_presumed_abort_duration,
      std::unique_ptr<blockchain::TwoPhaseCommit> blockchain)
      : default_presumed_abort_duration_(default_presumed_abort_duration),
        blockchain_(blockchain.release()) {}

  grpc::Status CommitAtomicTransaction(
      grpc::ServerContext *context,
      const CommitAtomicTransactionRequest *request,
      CommitAtomicTransactionResponse *response) override;

  grpc::Status GetTransactionResult(
      grpc::ServerContext *context, const GetTransactionResultRequest *request,
      GetTransactionResultResponse *response) override;

 private:
  // The virtual methods are so that they can be mocked out for testing.
  virtual void PrepareCohortTransaction(
      const std::string &transaction_id, const common::Namespace &namespace_,
      const cohort::PrepareTransactionRequest &request,
      const grpc::ServerContext &context);
  virtual grpc::Status GetResultsFromCohort(
      const common::Namespace &namespace_,
      const cohort::GetTransactionResultRequest &request,
      grpc::ClientContext &context,
      cohort::GetTransactionResultResponse &response);
  virtual absl::Time Now();
  virtual absl::Status StartVoting(
      const std::string &transaction_id,
      const google::protobuf::Timestamp &presumed_abort_time,
      size_t num_cohorts);
  virtual absl::StatusOr<blockchain::VotingDecision> GetVotingDecision(
      const std::string &transaction_id);
  virtual cohort::Cohort::StubInterface &GetCohortStub(
      const common::Namespace &namespace_);
  virtual bool SortCohortRequests() { return false; }

  void SendCohortPrepareRequests(
      const std::string &transaction_id,
      const std::vector<internal::SubTransaction> &sub_transactions,
      const grpc::ServerContext &context);
  grpc::Status UpdateResponseForSingleCohortTransaction(
      const std::string &transaction_id, const grpc::ServerContext &context);
  // Updates response to client when the blockchain says the transaction
  // aborted.
  void UpdateAbortedResponseFromCohorts(const std::string &transaction_id,
                                        const grpc::ServerContext &context);
  // Updates response to client when the blockchain says the transaction
  // committed.
  void UpdateCommittedResponseFromCohorts(const std::string &transaction_id,
                                          const grpc::ServerContext &context);
  // Garbage collects metadata for a transaction once the final response is
  // known.
  void CleanUpTransactionMetadata(const std::string &transaction_id);

  absl::flat_hash_map<std::string,
                      std::unique_ptr<cohort::Cohort::StubInterface>>
      cohort_by_namespace_;
  absl::flat_hash_map<std::string, internal::TransactionMetadata>
      metadata_by_transaction_;
  absl::flat_hash_map<std::string, internal::TransactionResponse>
      response_by_transaction_;
  absl::Duration default_presumed_abort_duration_;
  std::unique_ptr<blockchain::TwoPhaseCommit> blockchain_;
};

}  // namespace coordinator

#endif  // SRC_COORDINATOR_COORDINATOR_SERVER_H_