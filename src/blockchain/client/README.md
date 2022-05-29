# Blockchain Client

This components runs a RPC server written in JS which talks to our deployed
TwoPhaseCommit smart contract.

## Setup

Before starting this blockchain client, first start a dev blockchain running on
`localhost:9545`:

```
$ bazel run //src/blockchain:deploy_truffle
...
> compile
...
> migrate
```

Check `contract address` and `account address` for later use.

## Run and Test Adapter Server

TODO: Add bazel support.

```
$ node adapter_server.js <PORT> <CONTRACT_ADDRESS> <ACCOUNT>
```

If you want to quickly test if the server is alive, run this command and expect
'Is Ok: true' as output:

```
$ node adapter_server_test.js
```

## Test Contract Client Library

```
$ bazel run //src/blockchain/client:run_contract_client_test
```
