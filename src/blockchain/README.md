# Blockchain

This folder contains the code of the Two Phase Commit smart contract and its
deployment, adaptor, etc.

## Background

Terms are:

- Solidify: a language to describe a smart contract.
- Truffle: a development environment for Solidify contracts.
- Ganache: a local blockchain simulator for Truffle to deploy to for testing.

## Overview

This folder contains the following components.

### 1. Contract

Two Phase Commit contract itself lives under `contracts` and its tests (both
unit-test written in Solidity and end-to-end test written in JavaScript) live
in `test`. Follow the below instruction to compile, test, and deploy.

### 2. Library Interface > Adapter Service > Contract Client

Once we've deployed the contract, we want to eventually provide a C++ interface
for upstream to use. As we don't have a good C++ client library for Ethereum
smart contracts, we implement with an adapter service (local RPC) to help adapt
C++ interface to JavaScript contract client:

```
[TwoPhaseCommit (C++ Library Interface)]
                       v
[TwoPhaseCommitAdapterService Client (C++ RPC Client)]
                       v
[TwoPhaseCommitAdapterService Server (JavaScript RPC Server)]
                       v
[TwoPhaseCommitClient (JavaScript Smart Contract Client)]
```

## Development

### Compile

```bash
$ bazel run //src/blockchain:compile_blockchain
```

### Test

```bash
$ bazel test //src/blockchain:test_blockchain
```

NOTE: Truffle supports writing tests in JavaScript which behaves more like an
integration test - we can consider this later.

### Deploy (on Truffle's dev network, can't interact with the contract)

```bash
$ bazel run //src/blockchain:deploy_truffle
...
> migrate
```

### Deploy (on local Ganache, can interact with the contract)

```
$ bazel run //src/blockchain:deploy_ganache_with_console
truffle(development)> let contract = await TwoPhaseCommit.deployed();
undefined
truffle(development)> contract.getHeartBeat();
true
```

## Reference

- [Truffle Documentation](https://trufflesuite.com/docs/truffle/)
