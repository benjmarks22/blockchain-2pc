#!/bin/bash
set -eEuo pipefail

export RUNFILES_DIR="$0.runfiles"
source "${RUNFILES_DIR}/bazel_tools/tools/bash/runfiles/runfiles.bash"

setup_truffle="$(rlocation __main__/src/blockchain/setup_truffle.sh)"
source $setup_truffle
$truffle compile