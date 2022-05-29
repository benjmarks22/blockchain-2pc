#!/bin/bash
set -eEuo pipefail

export RUNFILES_DIR="$0.runfiles"
source "${RUNFILES_DIR}/bazel_tools/tools/bash/runfiles/runfiles.bash"

deploy_ganache="$(rlocation __main__/src/blockchain/deploy_ganache.sh)"
source $deploy_ganache
$truffle console