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

struct TransactionMetadata {
  common::TransactionConfig config;
  std::vector<std::unique_ptr<grpc::ClientContext>> contexts;
  std::vector<common::Namespace> get_cohort_namespaces;
  // This allows skipping the blockchain since there's no need to agree on the
  // commit decision when only one cohort is involved.
  absl::optional<common::Namespace> single_cohort_namespace;
  // This indicates that the transaction may have been sent to all the cohorts
  // and thus might commit.
  bool possibly_sent_to_all_cohorts;
  blockchain::TwoPhaseCommit::VotingDecision decision;
  absl::flat_hash_set<std::string> get_cohorts_already_responded;
};

struct TransactionResponse {
  bool complete;
  GetTransactionResultResponse response;
};

class CoordinatorServer : public Coordinator::Service {
 public:
  explicit CoordinatorServer(absl::Duration default_presumed_abort_duration)
      : default_presumed_abort_duration_(default_presumed_abort_duration) {}

  grpc::Status CommitAtomicTransaction(
      grpc::ServerContext *context,
      const CommitAtomicTransactionRequest *request,
      CommitAtomicTransactionResponse *response) override;

  grpc::Status GetTransactionResult(
      grpc::ServerContext *context, const GetTransactionResultRequest *request,
      GetTransactionResultResponse *response) override;

  // These are protected so they can be mocked out in testing.
 protected:
  virtual void PrepareCohortTransaction(
      const std::string &transaction_id, const common::Namespace &ns,
      const cohort::PrepareTransactionRequest &request,
      const grpc::ServerContext &context);
  virtual std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<
      cohort::GetTransactionResultResponse>>
  AsyncGetResultsFromCohort(const common::Namespace &ns,
                            const cohort::GetTransactionResultRequest &request,
                            grpc::ClientContext &context);
  virtual grpc::Status FinishAsyncGetResultsFromCohort(
      std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<
          cohort::GetTransactionResultResponse>> &async_response,
      cohort::GetTransactionResultResponse &response);
  virtual absl::Time Now();
  virtual absl::Status StartVoting(
      const std::string &transaction_id,
      const google::protobuf::Timestamp &presumed_abort_time,
      size_t num_cohorts);
  virtual absl::StatusOr<blockchain::TwoPhaseCommit::VotingDecision>
  GetVotingDecision(const std::string &transaction_id);

 private:
  virtual cohort::Cohort::StubInterface &GetCohortStub(
      const common::Namespace &ns);
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
  absl::flat_hash_map<std::string, TransactionMetadata>
      metadata_by_transaction_;
  absl::flat_hash_map<std::string, TransactionResponse>
      response_by_transaction_;
  absl::Duration default_presumed_abort_duration_;
};

}  // namespace coordinator

#endif  // SRC_COORDINATOR_COORDINATOR_SERVER_H_