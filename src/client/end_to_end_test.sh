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

function start_blockchain_with_ganache() {
    rm /tmp/ganache_log.txt
    deploy_ganache="$(rlocation __main__/src/blockchain/deploy_ganache_command.bash)"
    $deploy_ganache $1 $2 $3 > /tmp/ganache.txt &
    while [ 1 ]
    do
        contract_address=$(grep "Contract created:" /tmp/ganache_log.txt | tail -n 1 | cut -d " " -f 5) || :
        if [ -n "$contract_address" ]
        then
            echo "$contract_address"
            return
        fi
        sleep 2
    done
}

function start_cohort() {
    cohort_port=$3
    adapter_server_port=$((cohort_port+500))
    fuser -k $cohort_port/tcp && sleep 1s
    fuser -k $adapter_server_port/tcp && sleep 1s
    rm -r "/tmp/data_$cohort_port"
    rm -r "/tmp/txn_responses_$cohort_port"
    args=()
    args+=(" $1 $2")
    args+=(" --contract_address=$4")
    args+=(" --adapter_server_port=$adapter_server_port")
    args+=(" --")
    args+=(" --port=$cohort_port")
    args+=(" --db_data_dir=/tmp/data_$cohort_port")
    args+=(" --db_txn_response_dir=/tmp/txn_responses_$cohort_port")
    $(rlocation __main__/src/cohort/start_cohort_with_blockchain_adapter_server_command.bash) ${args[@]}
}
function start_all_cohorts() {
    for cohort_port in {60000..60009}
    do
        start_cohort $1 $2 $cohort_port $3 || exit 1
    done
}

function start_coordinator() {
    coordinator_port=$3
    adapter_server_port=$((coordinator_port+500))
    fuser -k $coordinator_port/tcp && sleep 1s
    fuser -k $adapter_server_port/tcp && sleep 1s
    args=()
    args+=(" $1 $2")
    args+=(" --contract_address=$4")
    args+=(" --adapter_server_port=$adapter_server_port")
    args+=(" --")
    args+=(" --port=$coordinator_port")
    $(rlocation __main__/src/coordinator/start_coordinator_with_blockchain_adapter_server_command.bash) ${args[@]}
}

function start_client() {
    $(rlocation __main__/src/client/client_main) --coordinator_port=$1 --request_file_prefix=$2
}

main() {
    contract_address=$(start_blockchain_with_ganache $1 $2 $3)
    start_all_cohorts $1 $2 $contract_address || exit 1
    coordinator_port="58000"
    start_coordinator $1 $2 $coordinator_port $contract_address || exit 1
    start_client $coordinator_port $4
}

main $1 $2 $3 $4