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

const transactionID = new Date().toISOString();  // current time
const timeoutTime = 1672560000;                  // unix_time: 1/1/2032 00:00PM

function main() {
  var adapterClient = new twoPhaseCommitAdapterProto.TwoPhaseCommitAdapter(
      'localhost:50051', grpc.credentials.createInsecure());

  adapterClient.getHeartBeat({}, function(err, response) {
    console.log('GetHeartBeat Response', response);
  });

  // Set timeout time as one hour later.
  console.log('StartVoting Start');
  adapterClient.startVoting(
      {
        transaction_id: transactionID,
        timeout_time: {seconds: timeoutTime, nanos: 0},
        cohorts: 3
      },
      function(err, response) {
        if (err) {
          console.log('StartVoting Error', err);
        }
        console.log('StartVoting Response', response)

        console.log('Vote 1 Start');
        adapterClient.vote(
            {transaction_id: transactionID, cohort_id: 0, ballot: 'COMMIT'},
            function(err, response) {
              if (err) {
                console.log('Vote 1 Error', err);
              }
              console.log('Vote 1 Response', response)

              console.log('Vote 2 Start');
              adapterClient.vote(
                  {
                    transaction_id: transactionID,
                    cohort_id: 1,
                    ballot: 'COMMIT'
                  },
                  function(err, response) {
                    if (err) {
                      console.log('Vote 2 Error', err);
                    }
                    console.log('Vote 2 Response', response)
                  });
            });
      });
}

main();
