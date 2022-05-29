// SPDX-License-Identifier: MIT
pragma solidity >=0.4.22 <0.9.0;
pragma experimental ABIEncoderV2;

contract TwoPhaseCommit {
    enum Ballot {
        UNKNOWN,
        COMMIT,
        ABORT
    }

    struct TransactionConfig {
        uint32 cohorts;
        uint256 vote_timeout_time;
    }

    // Voting states of each transaction.
    // Mapping: transaction_id -> ballot[cohort_id].
    mapping(string => Ballot[]) private transaction_states;

    // Configurations of each transaction.
    mapping(string => TransactionConfig) private transaction_configs;

    // Used by test only - mocked time for now. 0 means unset and should be
    // no-op to any contract behavior.
    uint256 private mock_now = 0;

    // Starts voting on a new transaction.
    function startVoting(
        string memory transaction_id,
        // Number of cohorts which will be identified with cohort_id = 0, 1,
        // ..., (cohorts -1).
        uint32 cohorts,
        // Format: unix timestamp.
        uint256 vote_timeout_time
    ) public {
        require(cohorts > 0);
        require(getNow() < vote_timeout_time);
        transaction_configs[transaction_id].cohorts = cohorts;
        transaction_configs[transaction_id]
            .vote_timeout_time = vote_timeout_time;
        // TODO(heronyang): This dynamic allocation fails if we run end-to-end.
        transaction_states[transaction_id] = new Ballot[](cohorts);
        for (uint32 i = 0; i < cohorts; i++) {
            transaction_states[transaction_id][i] = Ballot.UNKNOWN;
        }
    }

    // Votes if a cohort can commit a transaction.
    function vote(
        string memory transaction_id,
        uint32 cohort_id,
        Ballot ballot
    ) public {
        require(
            getNow() < transaction_configs[transaction_id].vote_timeout_time
        );
        require(transaction_configs[transaction_id].cohorts > cohort_id);
        require(ballot != Ballot.UNKNOWN);
        transaction_states[transaction_id][cohort_id] = ballot;
    }

    enum VotingDecisionOption {
        UNKNOWN, // Saved for unexpected unset value only.
        PENDING,
        COMMIT,
        ABORT
    }

    struct VotingDecision {
        VotingDecisionOption option;
        string reason;
    }

    // Gets the voting decision of a transaction.
    function getVotingDecision(string memory transaction_id)
        public
        view
        returns (VotingDecision memory)
    {
        require(transaction_configs[transaction_id].cohorts > 0);

        uint32 cohorts = transaction_configs[transaction_id].cohorts;
        uint32 votes = 0;
        for (uint32 i = 0; i < cohorts; i++) {
            if (transaction_states[transaction_id][i] == Ballot.COMMIT) {
                votes++;
            }
            if (transaction_states[transaction_id][i] == Ballot.ABORT) {
                return
                    VotingDecision({
                        option: VotingDecisionOption.ABORT,
                        reason: "Cohort voted to abort."
                    });
            }
        }

        if (cohorts == votes) {
            return
                VotingDecision({
                    option: VotingDecisionOption.COMMIT,
                    reason: "Sufficient vote collected before timeout."
                });
        }

        // As this point, we have insufficient votes to make a decision, so we
        // check if we've timed out.
        if (getNow() >= transaction_configs[transaction_id].vote_timeout_time) {
            return
                VotingDecision({
                    option: VotingDecisionOption.ABORT,
                    reason: "Insufficient vote after timeout."
                });
        }
        return
            VotingDecision({
                option: VotingDecisionOption.PENDING,
                reason: "Insufficient vote before timeout."
            });
    }

    // Called by test only - sets the mock time for now.
    function setMockNow(uint256 new_mock_now) public {
        mock_now = new_mock_now;
    }

    function getNow() private view returns (uint256) {
        if (mock_now > 0) {
            return mock_now;
        }
        return now;
    }

    function getHeartBeat() public pure returns (bool) {
        return true;
    }
}
