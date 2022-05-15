#!/bin/bash

function init_binary_dir () {
    BINARY_DIR="$1"
    mkdir -p "${BINARY_DIR}"
}

function install () {
    BINARY_DIR="$1"
    BINARY_NAME="$2"
    URL="$3"
    if ! [ -x "$(command -v ${BINARY_NAME})" ]; then
        curl --location --fail "${URL}" --output "/tmp/${BINARY_NAME}" || exit 1
        sudo mv "/tmp/${BINARY_NAME}" "${BINARY_DIR}/${BINARY_NAME}"
        chmod +x "${BINARY_DIR}/${BINARY_NAME}"
    fi
}

BINARY_DIR="/usr/local/bin"
sudo apt-get update || exit 2
sudo apt-get install -y clang-format clang-tidy curl git gcc g++ python3 python3-pip || exit 3
install ${BINARY_DIR} bazel "https://github.com/bazelbuild/bazelisk/releases/download/v1.11.0/bazelisk-linux-amd64"
install ${BINARY_DIR} buildifier "https://github.com/bazelbuild/buildtools/releases/download/5.1.0/buildifier-linux-amd64"
python3 -m pip install pre-commit
export PATH="$PATH:~/.local/bin/"   # pre-commit might not locate in $PATH already.
pre-commit install
