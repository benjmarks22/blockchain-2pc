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

TODO(heronyang): Add this part.

## Run Demo (No RPC Server / Trying Library Code Only)

```
$ bazel run //src/blockchain/client:main_client
```
