# Blockchain

This folder contains the code of the Two Phase Commit smart contract and its
deployment, adaptor, etc.

## Background

Terms are:

- Solidify: a language to describe a smart contract.
- Truffle: a development environment for Solidify contracts.
- Ganache: a local blockchain simulator for Truffle to deploy to for testing.

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
