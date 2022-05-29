#!/bin/bash
set -eEuo pipefail

export RUNFILES_DIR="$0.runfiles"
source "${RUNFILES_DIR}/bazel_tools/tools/bash/runfiles/runfiles.bash"

setup_truffle="$(rlocation __main__/src/blockchain/setup_truffle.sh)"
source $setup_truffle
ganache="$(rlocation __main__/$3)"
# $ganache --port=9545 &
# $truffle migrate
$truffle develop &
# $truffle console
cd $(rlocation __main__/src/blockchain/client)
node $(rlocation __main__/src/blockchain/client/main.js)