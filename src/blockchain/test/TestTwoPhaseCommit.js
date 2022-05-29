// End-to-end test for TwoPhaseCommit smart contract.
const TwoPhaseCommit = artifacts.require("./TwoPhaseCommit.sol");

contract("TwoPhaseCommit", accounts => {
  it("should get heartbeat", async () => {
    const twoPhaseCommit = await TwoPhaseCommit.deployed();
    const storedString = await twoPhaseCommit.getHeartBeat();
    assert.equal(storedString, true, "Failed to get heartbeat.");
  });
});

