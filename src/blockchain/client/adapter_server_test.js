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

const transactionId = new Date().toISOString();  // current time
const timeoutTime = 1672560000;                  // unix_time: 1/1/2032 00:00PM

function log(err, response, label) {
  if (err) {
    console.log(label + ' Error', err);
  }
  console.log(label + ' Repsonse', response);
}

/*
 * Expected Output
 * ---------------
 *  StartVoting Start
 *  GetHeartBeat Response { is_ok: true }
 *  StartVoting Repsonse {}
 *  Vote 1 Start
 *  Vote 1 Repsonse {}
 *  Vote 2 Start
 *  Vote 2 Repsonse {}
 *  GetVotingDecision Start
 *  GetVotingDecision Repsonse {
 *    decision_option: 'VOTING_DECISION_COMMIT',
 *    reason: 'Sufficient vote collected before timeout.'
 *  }
 */
function main() {
  var adapterClient = new twoPhaseCommitAdapterProto.TwoPhaseCommitAdapter(
      'localhost:50051', grpc.credentials.createInsecure());

  adapterClient.getHeartBeat({}, function(err, response) {
    console.log('GetHeartBeat Response', response);
  });

  console.log('StartVoting Start');
  adapterClient.startVoting(
      {
        transaction_id: transactionId,
        timeout_time: {seconds: timeoutTime, nanos: 0},
        cohorts: 2
      },
      function(err, response) {
        log(err, response, 'StartVoting');

        console.log('Vote 1 Start');
        adapterClient.vote(
            {
              transaction_id: transactionId,
              cohort_id: 0,
              ballot: 'BALLOT_COMMIT'
            },
            function(err, response) {
              log(err, response, 'Vote 1');

              console.log('Vote 2 Start');
              adapterClient.vote(
                  {
                    transaction_id: transactionId,
                    cohort_id: 1,
                    ballot: 'BALLOT_COMMIT'
                  },
                  function(err, response) {
                    log(err, response, 'Vote 2');

                    console.log('GetVotingDecision Start');
                    adapterClient.getVotingDecision(
                        {transaction_id: transactionId},
                        function(err, response) {
                          log(err, response, 'GetVotingDecision');
                        })
                  });
            });
      });
}

main();
