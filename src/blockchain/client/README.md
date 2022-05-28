# Blockchain Client

This components runs a RPC server written in JS which talks to our deployed
TwoPhaseCommit smart contract.

## Setup

Before starting this blockchain client, first start a dev blockchain running on
`localhost:7545`:

```
bazel run //src/blockchain:deploy_ganache
```

## Run Blockchain Client Server

TODO(heronyang): Add this part.

## Run Demo (No RPC Server / Trying Library Code Only)

```
$ node demo.js
```

TODO(heronyang): Add bazel support.
