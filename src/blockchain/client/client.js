const fs = require('fs');

module.exports = class TwoPhaseCommitClient {
  constructor(contract_json_file, contract_provider, contract_address) {
    const abi = JSON.parse(fs.readFileSync(contract_json_file)).abi;
    var Contract = require('web3-eth-contract');
    Contract.setProvider(contract_provider);
    this.contract = new Contract(abi, contract_address);
  }

  async getHeartBeat() {
    return this.contract.methods.getHeartBeat().call();
  }
};
