#!/bin/bash
set -eEuo pipefail

export RUNFILES_DIR="$0.runfiles"
source "${RUNFILES_DIR}/bazel_tools/tools/bash/runfiles/runfiles.bash"

deploy_ganache="$(rlocation __main__/src/blockchain/deploy_ganache.sh)"
source $deploy_ganache
contract_address=$(grep "Contract created:" ganache_log.txt | tail -n 1 | cut -d " " -f 5)
# Don't change this since it's the first one in the deterministic ganache client.
account='0x90F8bf6A479f320ead074411a4B0e7944Ea8c9C1'
cd $(rlocation __main__/src/blockchain/client)
node $(rlocation __main__/src/blockchain/client/contract_client_test.js) $port $contract_address $account
