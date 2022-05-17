#!/bin/bash
set -eEuo pipefail

setup_truffle="$(rlocation __main__/src/blockchain/setup_truffle.sh)"
source $setup_truffle
ganache="$(rlocation __main__/$3)"
$ganache --port=7545 &
$truffle test