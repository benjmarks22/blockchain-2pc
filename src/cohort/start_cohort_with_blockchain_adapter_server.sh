#!/bin/bash
set -eEuo pipefail

# --- begin runfiles.bash initialization ---
if [[ ! -d "${RUNFILES_DIR:-/dev/null}" && ! -f "${RUNFILES_MANIFEST_FILE:-/dev/null}" ]]; then
    if [[ -f "$0.runfiles_manifest" ]]; then
      export RUNFILES_MANIFEST_FILE="$0.runfiles_manifest"
    elif [[ -f "$0.runfiles/MANIFEST" ]]; then
      export RUNFILES_MANIFEST_FILE="$0.runfiles/MANIFEST"
    elif [[ -f "$0.runfiles/bazel_tools/tools/bash/runfiles/runfiles.bash" ]]; then
      export RUNFILES_DIR="$0.runfiles"
    fi
fi
if [[ -f "${RUNFILES_DIR:-/dev/null}/bazel_tools/tools/bash/runfiles/runfiles.bash" ]]; then
  source "${RUNFILES_DIR}/bazel_tools/tools/bash/runfiles/runfiles.bash"
elif [[ -f "${RUNFILES_MANIFEST_FILE:-/dev/null}" ]]; then
  source "$(grep -m1 "^bazel_tools/tools/bash/runfiles/runfiles.bash " \
            "$RUNFILES_MANIFEST_FILE" | cut -d ' ' -f 2-)"
else
  echo >&2 "ERROR: cannot find @bazel_tools//tools/bash/runfiles:runfiles.bash"
  exit 1
fi
# --- end runfiles.bash initialization ---

contract_address=$4
adapter_server_port=${5:-61362}
cohort_port=${6:-51000}
# Don't change this since it's the first one in the deterministic ganache client.
account=${7:-'0x90F8bf6A479f320ead074411a4B0e7944Ea8c9C1'}
blockchain_port=${8:-7545}
db_data_dir=${9:-'/tmp/data'}
db_txn_response_dir=${10:-'/tmp/txn_responses'}
db_thread_ratio=${11:-0.75}
$(rlocation __main__/src/blockchain/client/run_adapter_server.sh) $contract_address $adapter_server_port $account $blockchain_port &
sleep 3s
$(rlocation __main__/src/cohort/cohort_server_main) --port=$cohort_port --blockchain_adapter_port=$adapter_server_port --db_data_dir=$db_data_dir --db_txn_response_dir=$db_txn_response_dir --db_thread_ratio=$db_thread_ratio