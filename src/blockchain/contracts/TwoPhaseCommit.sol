// SPDX-License-Identifier: MIT
pragma solidity >=0.4.22 <0.9.0;

contract TwoPhaseCommit {
    enum Ballot {
        UNKNOWN, // Saved for unexpected unset value only.
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

    // Starts voting on a new transaction.
    function startVoting(
        string memory transaction_id,
        // Number of cohorts which will be identified with cohort_id = 0, 1,
        // ..., (cohorts -1).
        uint32 cohorts,
        // Format: unix timestamp.
        uint256 vote_timeout_time
    ) public {
        transaction_configs[transaction_id].cohorts = cohorts;
        transaction_configs[transaction_id]
            .vote_timeout_time = vote_timeout_time;
        // TODO(heronyang): This dynamic allocation fails if we run end-to-end.
        transaction_states[transaction_id] = new Ballot[](cohorts);
        for (uint32 i = 0; i < cohorts; i++) {
            transaction_states[transaction_id][i] = Ballot.ABORT;
        }
    }

    // Votes if a cohort can commit a transaction.
    function vote(
        string memory transaction_id,
        uint32 cohort_id,
        Ballot ballot
    ) public {
        transaction_states[transaction_id][cohort_id] = ballot;
    }

    enum VotingDecision {
        UNKNOWN, // Saved for unexpected unset value only.
        PENDING,
        COMMIT,
        ABORT
    }

    // Gets the voting decision of a transaction.
    function getVotingDecision(string memory transaction_id)
        public
        view
        returns (VotingDecision)
    {
        uint32 cohorts = transaction_configs[transaction_id].cohorts;
        uint32 votes = 0;
        for (uint32 i = 0; i < cohorts; i++) {
            if (transaction_states[transaction_id][i] == Ballot.COMMIT) {
                votes++;
            }
        }
        if (cohorts == votes && cohorts != 0) {
            return VotingDecision.COMMIT;
        }
        return VotingDecision.UNKNOWN;
    }

    function getHeartBeat() public pure returns (bool) {
        return true;
    }
}
