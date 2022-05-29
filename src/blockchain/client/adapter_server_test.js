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
var twoPhaseCommitAdapterProto =
    grpc.loadPackageDefinition(packageDefinition).blockchain;

function main() {
  var adapterClient = new twoPhaseCommitAdapterProto.TwoPhaseCommitAdapter(
      'localhost:50051', grpc.credentials.createInsecure());
  /*
  adapterClient.getHeartBeat({}, function(err, response) {
    console.log('Is Ok:', response.is_ok);
  });
  */
  // Set timeout time as one hour later.
  const transaction_id =
      new Date().toISOString();     // use current time as transaction id.
  const timeout_time = 1672560000;  // unix_time: 1/1/2032 00:00PM
  adapterClient.startVoting(
      {
        transaction_id: transaction_id,
        timeout_time: {seconds: timeout_time, nanos: 0},
        cohorts: 3
      },
      function(err, response) {
        if (err) {
          console.log('StartVoting Error', err);
        }
        console.log('StartVoting Response', response)
      });
}

main();
