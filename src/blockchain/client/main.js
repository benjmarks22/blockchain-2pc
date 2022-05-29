// This script calls TwoPhaseCommitClient to demostrate a successful 2PC case.
const args = process.argv.slice(2);

const blockchain_uri = 'ws://localhost:' + args[0];
const contract_addr = args[1];
const shared_account = args[2];

const timeout_time = 1672560000;  // unix_time: 1/1/2032 00:00PM
const transaction_id =
    new Date().toISOString();  // use current time as transaction id.

var TwoPhaseCommitClient = require('./client.js');
var client = new TwoPhaseCommitClient(
    '../build/contracts/TwoPhaseCommit.json', blockchain_uri, contract_addr);
client.getHeartBeat(console.log);
client.startVoting(shared_account, transaction_id, 2, timeout_time, () => {
  client.vote(shared_account, transaction_id, 0, 1, () => {
    console.log('Client 1st Voted.');
    client.getVotingDecision(
        transaction_id,
        (result) => {
            console.log('Got Voting Decision (Expect PENDING).', result)});
    client.vote(shared_account, transaction_id, 1, 1, () => {
      console.log('Client 2nd Voted!');
      client.getVotingDecision(transaction_id, (result) => {
        console.log('Got Voting Decision (Expect COMMIT).', result);
        process.exit();
      });
    });
  });
});
