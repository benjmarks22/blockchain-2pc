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

function main() {
  var client = new two_phase_commit_adapter_proto.TwoPhaseCommitAdapter(
      'localhost:50051', grpc.credentials.createInsecure());
  client.getHeartBeat({}, function(err, response) {
    console.log('Is Ok:', response.is_ok);
  });
}

main();
