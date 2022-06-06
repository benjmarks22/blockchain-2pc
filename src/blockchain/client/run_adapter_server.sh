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
compile_script=$(rlocation __main__/src/blockchain/compile_command.bash)
$compile_script $1 $2
shift
shift
adapter_server=$(rlocation __main__/src/blockchain/client/adapter_server_command.bash)
contract_address='0xCfEB869F69431e42cdB54A4F4f105C19C080A601'
adapter_server_port=61362
# Don't change this since it's the first one in the deterministic ganache client.
account='0x90F8bf6A479f320ead074411a4B0e7944Ea8c9C1'
blockchain_port=7545
extra_args=(" ")
while [ $# -gt 0 ]
do
  flag=${1%%=*}
  value="${1#*=}"
  case "$flag" in
    --contract_address*) contract_address="$value";;
    --adapter_server_port*) adapter_server_port="$value";;
    --account*) account="$value";;
    --blockchain_port*) blockchain_port="$value";;
    *) extra_args+=" $1";;
  esac
  shift
done
$adapter_server $blockchain_port $contract_address $account $adapter_server_port $extra_args{@} &
