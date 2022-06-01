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
adapter_server_args=()
coordinator_server_args=()
# Needs to be consistent between JS adapter server and coordinator server.
adapter_server_port=51560
while [ $# -gt 0 ]
do
  case "$1" in
    --adapter_server_port*) adapter_server_port=${1#*=};;
    --) shift; break;;
    *) adapter_server_args+=" $1";;
  esac
  shift
done
while [ $# -gt 0 ]
do
  coordinator_server_args+=" $1"
  shift
done
adapter_server_args+=" --adapter_server_port=$adapter_server_port"
coordinator_server_args+=" --blockchain_adapter_port=$adapter_server_port"
$(rlocation __main__/src/blockchain/client/run_adapter_server_command.bash) $adapter_server_args
$(rlocation __main__/src/coordinator/coordinator_server_main) $coordinator_server_args &