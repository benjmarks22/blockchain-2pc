// SPDX-License-Identifier: MIT
// Unit-test for TwoPhaseCommit smart contract.
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

        Assert.equal(
            two_phase_commit.getVotingDecision("t1"),
            "2:Sufficient vote collected before timeout.",
            "Expect getting a commit decision of a transaction after all "
            "cohorts have voted commit."
        );
    }

    function testPendingTransaction() public {
        TwoPhaseCommit two_phase_commit = new TwoPhaseCommit();
        two_phase_commit.setMockNow(current_time);
        two_phase_commit.startVoting("t1", 3, future_time);
        two_phase_commit.vote("t1", 0, TwoPhaseCommit.Ballot.COMMIT);
        two_phase_commit.vote("t1", 1, TwoPhaseCommit.Ballot.COMMIT);

        Assert.equal(
            two_phase_commit.getVotingDecision("t1"),
            "1:Insufficient vote before timeout.",
            "Expect getting a pending decision ('insufficient vote before timeout')"
        );
    }

    function testAbortTransactionWithInsufficientVote() public {
        TwoPhaseCommit two_phase_commit = new TwoPhaseCommit();
        two_phase_commit.setMockNow(current_time);
        two_phase_commit.startVoting("t1", 3, future_time);
        two_phase_commit.vote("t1", 0, TwoPhaseCommit.Ballot.COMMIT);
        two_phase_commit.vote("t1", 1, TwoPhaseCommit.Ballot.COMMIT);
        two_phase_commit.vote("t1", 2, TwoPhaseCommit.Ballot.ABORT);

        Assert.equal(
            two_phase_commit.getVotingDecision("t1"),
            "3:Cohort voted to abort.",
            "Expect getting an abort decision ('cohort voted to abort')."
        );
    }

    function testAbortTransactionWithTimeout() public {
        TwoPhaseCommit two_phase_commit = new TwoPhaseCommit();
        two_phase_commit.setMockNow(current_time);
        two_phase_commit.startVoting("t1", 3, future_time);
        two_phase_commit.vote("t1", 0, TwoPhaseCommit.Ballot.COMMIT);
        two_phase_commit.vote("t1", 1, TwoPhaseCommit.Ballot.COMMIT);

        two_phase_commit.setMockNow(future_time);

        Assert.equal(
            two_phase_commit.getVotingDecision("t1"),
            "3:Insufficient vote after timeout.",
            "Expect getting an abort decision ('insufficient vote after timeout')."
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
            two_phase_commit.getVotingDecision("t1"),
            "1:Insufficient vote before timeout.",
            "Expect getting a pending decision of a transaction (t1)."
        );

        two_phase_commit.vote("t2", 0, TwoPhaseCommit.Ballot.COMMIT);
        Assert.equal(
            two_phase_commit.getVotingDecision("t2"),
            "1:Insufficient vote before timeout.",
            "Expect getting a pending decision of a transaction (t2)."
        );

        two_phase_commit.vote("t3", 0, TwoPhaseCommit.Ballot.ABORT);
        Assert.equal(
            two_phase_commit.getVotingDecision("t3"),
            "3:Cohort voted to abort.",
            "Expect getting a abort decision of a transaction (t3)."
        );

        two_phase_commit.vote("t1", 0, TwoPhaseCommit.Ballot.COMMIT);
        Assert.equal(
            two_phase_commit.getVotingDecision("t1"),
            "2:Sufficient vote collected before timeout.",
            "Expect getting a commit decision of a transaction (t1)."
        );

        two_phase_commit.setMockNow(future_time);
        Assert.equal(
            two_phase_commit.getVotingDecision("t2"),
            "3:Insufficient vote after timeout.",
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
