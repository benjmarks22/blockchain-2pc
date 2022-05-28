// SPDX-License-Identifier: MIT
// Unit-test for TwoPhaseCommit smart contract.
pragma solidity >=0.4.22 <0.9.0;

import "truffle/Assert.sol";
import "../contracts/TwoPhaseCommit.sol";

contract TestTwoPhaseCommit {
    function testCommitTransaction() public {
        TwoPhaseCommit two_phase_commit = new TwoPhaseCommit();
        two_phase_commit.startVoting(
            "t1",
            3,
            1956571200 /*unix_time: Thursday, January 1, 2032 12:00:00 PM*/
        );
        two_phase_commit.vote("t1", 0, TwoPhaseCommit.Ballot.COMMIT);
        two_phase_commit.vote("t1", 1, TwoPhaseCommit.Ballot.COMMIT);
        two_phase_commit.vote("t1", 2, TwoPhaseCommit.Ballot.COMMIT);
        Assert.equal(
            int256(TwoPhaseCommit.VotingDecision.COMMIT),
            int256(two_phase_commit.getVotingDecision("t1")),
            "Expect getting a commit decision of a transaction after all "
            "cohorts have voted commit."
        );
    }

    function testAbortTransactionWithInsufficientVote() public {
        // TODO(heronyang).
    }

    function testAbortTransactionWithTimeout() public {
        // TODO(heronyang).
    }

    function testTwoTransactions() public {
        // TODO(heronyang).
    }

    function testVoteOnUnknownTransaction() public {
        // TODO(heronyang).
    }

    function testGetVotingDecisionOnUnknownTransaction() public {
        // TODO(heronyang).
    }

    function testHeartBeat() public {
        TwoPhaseCommit two_phase_commit = new TwoPhaseCommit();
        Assert.isTrue(
            two_phase_commit.getHeartBeat(),
            "TwoPhaseCommit contract should return valid heart beat."
        );
    }
}
