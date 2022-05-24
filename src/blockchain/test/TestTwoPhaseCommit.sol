// SPDX-License-Identifier: MIT
pragma solidity >=0.4.22 <0.9.0;
pragma experimental ABIEncoderV2;

import "truffle/Assert.sol";
import "../contracts/TwoPhaseCommit.sol";

contract TestTwoPhaseCommit {
    uint256 current_time = 1641024000; /*unix_time: 1/1/2022 00:00PM */
    uint256 future_time = 1672560000; /*unix_time: 1/1/2032 00:00PM*/

    function testCommitTransaction() public {
        TwoPhaseCommit two_phase_commit = new TwoPhaseCommit();
        two_phase_commit.setMockNow(current_time);
        two_phase_commit.startVoting("t1", 3, future_time);
        two_phase_commit.vote("t1", 0, TwoPhaseCommit.Ballot.COMMIT);
        two_phase_commit.vote("t1", 1, TwoPhaseCommit.Ballot.COMMIT);
        two_phase_commit.vote("t1", 2, TwoPhaseCommit.Ballot.COMMIT);

        TwoPhaseCommit.VotingDecision memory decision = two_phase_commit
            .getVotingDecision("t1");
        Assert.equal(
            int256(TwoPhaseCommit.VotingDecisionOption.COMMIT),
            int256(decision.option),
            "Expect getting a commit decision of a transaction after all "
            "cohorts have voted commit."
        );
    }

    function testAbortTransactionWithInsufficientVote() public {
        TwoPhaseCommit two_phase_commit = new TwoPhaseCommit();
        two_phase_commit.setMockNow(current_time);
        two_phase_commit.startVoting("t1", 3, future_time);
        two_phase_commit.vote("t1", 0, TwoPhaseCommit.Ballot.COMMIT);
        two_phase_commit.vote("t1", 1, TwoPhaseCommit.Ballot.COMMIT);
        two_phase_commit.vote("t1", 2, TwoPhaseCommit.Ballot.ABORT);

        TwoPhaseCommit.VotingDecision memory decision = two_phase_commit
            .getVotingDecision("t1");
        Assert.equal(
            int256(TwoPhaseCommit.VotingDecisionOption.ABORT),
            int256(decision.option),
            "Expect getting an abort decision of a transaction after one "
            "cohort voted abort."
        );
        Assert.equal(
            "Cohort voted to abort.",
            decision.reason,
            "Expect getting 'cohort voted to abort' as reason."
        );
    }

    function testAbortTransactionWithTimeout() public {
        TwoPhaseCommit two_phase_commit = new TwoPhaseCommit();
        two_phase_commit.setMockNow(current_time);
        two_phase_commit.startVoting("t1", 3, future_time);
        two_phase_commit.vote("t1", 0, TwoPhaseCommit.Ballot.COMMIT);
        two_phase_commit.vote("t1", 1, TwoPhaseCommit.Ballot.COMMIT);

        two_phase_commit.setMockNow(future_time);

        TwoPhaseCommit.VotingDecision memory decision = two_phase_commit
            .getVotingDecision("t1");
        Assert.equal(
            int256(TwoPhaseCommit.VotingDecisionOption.ABORT),
            int256(decision.option),
            "Expect getting an abort decision of a transaction with "
            "insufficient vote after timeout."
        );
        Assert.equal(
            "Insufficient vote after timeout.",
            decision.reason,
            "Expect getting 'insufficient vote after timeout' as reason."
        );
    }

    function testPendingTransaction() public {
        TwoPhaseCommit two_phase_commit = new TwoPhaseCommit();
        two_phase_commit.setMockNow(current_time);
        two_phase_commit.startVoting("t1", 3, future_time);
        two_phase_commit.vote("t1", 0, TwoPhaseCommit.Ballot.COMMIT);
        two_phase_commit.vote("t1", 1, TwoPhaseCommit.Ballot.COMMIT);

        TwoPhaseCommit.VotingDecision memory decision = two_phase_commit
            .getVotingDecision("t1");
        Assert.equal(
            int256(TwoPhaseCommit.VotingDecisionOption.PENDING),
            int256(decision.option),
            "Expect getting a pending decision of a transaction with "
            "insufficient vote before timeout."
        );
        Assert.equal(
            "Insufficient vote before timeout.",
            decision.reason,
            "Expect getting 'insufficient vote before timeout' as reason."
        );
    }

    function testMultipleTransactions() public {
        // Scenario: t1 commits, t2 aborts with timeout, t3 aborts with vote.
        TwoPhaseCommit two_phase_commit = new TwoPhaseCommit();
        two_phase_commit.setMockNow(current_time);

        two_phase_commit.startVoting("t1", 2, future_time);
        two_phase_commit.startVoting("t2", 2, future_time);
        two_phase_commit.startVoting("t3", 2, future_time);

        two_phase_commit.vote("t1", 1, TwoPhaseCommit.Ballot.COMMIT);
        Assert.equal(
            int256(TwoPhaseCommit.VotingDecisionOption.PENDING),
            int256(two_phase_commit.getVotingDecision("t1").option),
            "Expect getting a pending decision of a transaction (t1)."
        );

        two_phase_commit.vote("t2", 0, TwoPhaseCommit.Ballot.COMMIT);
        Assert.equal(
            int256(TwoPhaseCommit.VotingDecisionOption.PENDING),
            int256(two_phase_commit.getVotingDecision("t2").option),
            "Expect getting a pending decision of a transaction (t2)."
        );

        two_phase_commit.vote("t3", 0, TwoPhaseCommit.Ballot.ABORT);
        Assert.equal(
            int256(TwoPhaseCommit.VotingDecisionOption.ABORT),
            int256(two_phase_commit.getVotingDecision("t3").option),
            "Expect getting a abort decision of a transaction (t3)."
        );

        two_phase_commit.vote("t1", 0, TwoPhaseCommit.Ballot.COMMIT);
        Assert.equal(
            int256(TwoPhaseCommit.VotingDecisionOption.COMMIT),
            int256(two_phase_commit.getVotingDecision("t1").option),
            "Expect getting a commit decision of a transaction (t1)."
        );

        two_phase_commit.setMockNow(future_time);
        Assert.equal(
            int256(TwoPhaseCommit.VotingDecisionOption.ABORT),
            int256(two_phase_commit.getVotingDecision("t2").option),
            "Expect getting a commit decision of a transaction (t2)."
        );
    }

    function testHeartBeat() public {
        TwoPhaseCommit two_phase_commit = new TwoPhaseCommit();
        Assert.isTrue(
            two_phase_commit.getHeartBeat(),
            "TwoPhaseCommit contract should return valid heart beat."
        );
    }
}
