const fs = require('fs');

/*
 * TwoPhaseCommitClient is the client of TwoPhaseCommit smart contract.
 */
module.exports = class TwoPhaseCommitClient {
  constructor(contract_json_file, contract_provider, contract_address) {
    var Contract = require('web3-eth-contract');
    Contract.setProvider(contract_provider);
    this.contract = new Contract(
        JSON.parse(fs.readFileSync(contract_json_file)).abi, contract_address);
  }

  startVoting(
      from_addr, transaction_id, cohorts, vote_timeout_time,
      on_success_callback) {
    this.contract.methods
        .startVoting(transaction_id, cohorts, vote_timeout_time)
        .send({from: from_addr})
        .then(function(receipt) {
          on_success_callback();
        })
        .catch(function(e) {
          console.log('StartVoting Error', e);
        });
  }

  vote(from_addr, transaction_id, cohort_id, ballot, on_success_callback) {
    this.contract.methods.vote(transaction_id, cohort_id, ballot)
        .send({from: from_addr})
        .then(function(receipt) {
          on_success_callback();
        })
        .catch(function(e) {
          console.log('Failed to vote');
        });
  }

  getVotingDecision(transaction_id, on_success_callback) {
    this.contract.methods.getVotingDecision(transaction_id)
        .call((e, result) => {
          if (e) {
            console.log('Get Voting Decision Error', e);
          } else {
            on_success_callback(result);
          }
        });
  }

  getHeartBeat(on_success_callback) {
    this.contract.methods.getHeartBeat().call((e, result) => {
      if (e) {
        console.log('Get HeartBeat Error', e);
      } else {
        on_success_callback(result);
      }
    });
  }
};
