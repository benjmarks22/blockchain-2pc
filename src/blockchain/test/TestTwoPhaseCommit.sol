// SPDX-License-Identifier: MIT
pragma solidity >=0.4.22 <0.9.0;

import "truffle/Assert.sol";
import "../contracts/TwoPhaseCommit.sol";

contract TestTwoPhaseCommit {
    function testHeartBeat() public {
        TwoPhaseCommit two_phase_commit = new TwoPhaseCommit();
        Assert.isTrue(two_phase_commit.getHeartBeat(), "TwoPhaseCommit contract should return valid heart beat.");
    }
}
