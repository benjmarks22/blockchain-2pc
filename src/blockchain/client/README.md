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

## Run Blockchain Client Server

TODO: Add bazel support.

```
$ node adapter_server.js
```

If you want to quickly test if the server is alive, run this command and expect
'Is Ok: true' as output:

```
$ node adapter_client_demo.js
```

## Run Demo (No RPC Server / Trying Library Code Only)

```
$ bazel run //src/blockchain/client:main_client
```
