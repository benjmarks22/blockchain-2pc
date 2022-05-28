var TwoPhaseCommitClient = require('./client.js');
var client = new TwoPhaseCommitClient(
    '../build/contracts/TwoPhaseCommit.json', 'ws://localhost:7545',
    '0xF276f52ba988ebd6ea286Dbf05BC931713c8cA22')
client.getHeartBeat().then(console.log);
