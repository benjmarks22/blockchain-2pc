const args = process.argv.slice(2);

const blockchainUri = 'ws://localhost:' + args[0];
const contractAddr = args[1];
const sharedAccount = args[2];

console.log('Blockchain URI', blockchainUri);
console.log('Contract Address', contractAddr);
console.log('Shared Account', sharedAccount);

const PROTO_PATH = __dirname + '/../proto/two_phase_commit_adapter.proto';

var grpc = require('@grpc/grpc-js');
var protoLoader = require('@grpc/proto-loader');
var packageDefinition = protoLoader.loadSync(PROTO_PATH, {
  keepCase: true,
  longs: String,
  enums: String,
  defaults: true,
  oneofs: true
});
var twoPhaseCommitAdapterProto =
    grpc.loadPackageDefinition(packageDefinition).blockchain;

var TwoPhaseCommitClient = require('./contract_client.js');
var contractClient = new TwoPhaseCommitClient(
    '../build/contracts/TwoPhaseCommit.json', blockchainUri, contractAddr);

function startVoting(call, callback) {
  console.log('Received: startVoting');
  contractClient.startVoting(
      sharedAccount, call.request.transaction_id, call.request.cohorts,
      call.request.timeout_time.seconds, () => {
        console.log('Done: startVoting');
        callback(null, {});
      });
}

function vote(call, callback) {
  console.log('Received: vote');
  // Convert to int (enum-based) defined in the smart contract (sol).
  var ballot_int = 0;
  if (call.request.ballot == 'COMMIT') {
    ballot_int = 1;
  } else if (call.request.ballot == 'ABORT') {
    ballot_int = 2;
  }
  contractClient.vote(
      sharedAccount, call.request.transaction_id, call.request.cohort_id,
      ballot_int, () => {
        console.log('Done: vote');
        callback(null, {});
      });
}

function getVotingDecision(call, callback) {
  console.log('Received: getVotingDecision');
  console.log('Done: getVotingDecision');
}

function getHeartBeat(call, callback) {
  console.log('Received: getHeartBeat');
  contractClient.getHeartBeat((result) => {
    callback(null, {is_ok: result});
    console.log('Done: getHeartBeat');
  });
}

function main() {
  var server = new grpc.Server();
  server.addService(twoPhaseCommitAdapterProto.TwoPhaseCommitAdapter.service, {
    startVoting: startVoting,
    vote: vote,
    getVotingDecision: getVotingDecision,
    getHeartBeat: getHeartBeat
  });
  server.bindAsync(
      '0.0.0.0:50051', grpc.ServerCredentials.createInsecure(), () => {
        server.start();
      });
}

main();
