# Blockchain

This folder contains the code of the Two Phase Commit smart contract and its
deployment, adaptor, etc.

## Background

Terms are:

- Solidify: a language to describe a smart contract.
- Truffle: a development environment for Solidify contracts.
- Ganache: a local blockchain simulator for Truffle to deploy to for testing.

## Installation

Install Truffle:

```bash
$ npm install -g truffle
```

Install Ganache: [link](https://trufflesuite.com/ganache/)

## Development

### Compile

```bash
$ truffle compile
```

### Test

```bash
$ truffle test
```

NOTE: Truffle supports writing tests in JavaScript which behaves more like an
integration test - we can consider this later.

### Deploy (on Truffle's dev network, can't interact with the contract)

```bash
$ truffle develop
...
> migrate
```

### Deploy (on local Ganache, can interact with the contract)

1. Launch Ganache > New > ... - Select `truffle-config.js` under this folder.
2. `$ truffle migrate` to deploy the contracts.
3. `$ truffle console` to interact with the contract. See below example:

```
$ truffle console
truffle(development)> let contract = await TwoPhaseCommit.deployed();
undefined
truffle(development)> contract.getHeartBeat();
true
```

## Reference

- [Truffle Documentation](https://trufflesuite.com/docs/truffle/)
