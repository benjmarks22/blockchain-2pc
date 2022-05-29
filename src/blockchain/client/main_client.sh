#!/bin/bash
set -eEuo pipefail

export RUNFILES_DIR="$0.runfiles"
source "${RUNFILES_DIR}/bazel_tools/tools/bash/runfiles/runfiles.bash"

deploy_ganache="$(rlocation __main__/src/blockchain/deploy_ganache.sh)"
source $deploy_ganache
contract_address=$(grep "Contract created:" ganache_log.txt | tail -n 1 | cut -d " " -f 5)
cd $(rlocation __main__/src/blockchain/client)
node $(rlocation __main__/src/blockchain/client/main.js) $port $contract_address