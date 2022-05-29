const args = process.argv.slice(2);

const blockchain_uri = 'ws://localhost:' + args[0];
const contract_addr = args[1];
const shared_account = args[2];

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
var two_phase_commit_adapter_proto =
    grpc.loadPackageDefinition(packageDefinition).blockchain;

var TwoPhaseCommitClient = require('./contract_client.js');
var contractClient = new TwoPhaseCommitClient(
    '../build/contracts/TwoPhaseCommit.json', blockchain_uri, contract_addr);

function getHeartBeat(call, callback) {
  contractClient.getHeartBeat((result) => {
    console.log('Processed: getHeartBeat');
    callback(null, {is_ok: result});
  });
}

function main() {
  var server = new grpc.Server();
  server.addService(
      two_phase_commit_adapter_proto.TwoPhaseCommitAdapter.service,
      {getHeartBeat: getHeartBeat});
  server.bindAsync(
      '0.0.0.0:50051', grpc.ServerCredentials.createInsecure(), () => {
        server.start();
      });
}

main();
