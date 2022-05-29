#!/bin/bash
set -eEuo pipefail

export RUNFILES_DIR="$0.runfiles"
source "${RUNFILES_DIR}/bazel_tools/tools/bash/runfiles/runfiles.bash"

setup_truffle="$(rlocation __main__/src/blockchain/setup_truffle.sh)"
source $setup_truffle
ganache="$(rlocation __main__/$3)"
port=${4:-7545}
# Kill the existing server if there is one.
# Note that it takes about 1 second for it to be killed.
fuser -k $port/tcp && sleep 1s || :
$ganache --server.port=$port --wallet.deterministic --chain.networkId=5777 2>&1 | tee ganache_log.txt &
$truffle migrate