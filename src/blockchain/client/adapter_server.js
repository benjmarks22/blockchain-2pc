var PROTO_PATH = __dirname + '/../proto/two_phase_commit_adapter.proto';

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

function getHeartBeat(call, callback) {
  callback(null, {is_ok: true});
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
