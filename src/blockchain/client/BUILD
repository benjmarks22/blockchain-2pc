load("@build_bazel_rules_nodejs//:index.bzl", "js_library", "nodejs_binary")
load("@com_github_ash2k_bazel_tools//multirun:def.bzl", "command")

js_library(
    name = "contract_client",
    srcs = ["contract_client.js"],
    deps = ["@blockchain_client_npm//web3-eth-contract"],
)

nodejs_binary(
    name = "adapter_server",
    data = [
        "adapter_server.js",
        ":contract_client",
        "//src/blockchain/contracts",
        "//src/blockchain/proto:two_phase_commit_adapter_proto_file",
        "@blockchain_client_npm//@grpc/grpc-js",
        "@blockchain_client_npm//@grpc/proto-loader",
    ],
    entry_point = "adapter_server.js",
    visibility = ["//visibility:public"],
)

js_library(
    name = "contract_client_test",
    srcs = ["contract_client_test.js"],
    deps = [":contract_client"],
)

sh_binary(
    name = "run_contract_client_test",
    srcs = [
        "contract_client_test.sh",
    ],
    args = [
        "$(location //src/blockchain:truffle-config)",
        "$(location @blockchain_npm//truffle/bin:truffle)",
        "$(location @blockchain_npm//ganache/bin:ganache)",
    ],
    data = [
        ":contract_client_test",
        "//src/blockchain:truffle-config",
        "//src/blockchain/contracts",
        "//src/blockchain/migrations",
        "@blockchain_npm//ganache/bin:ganache",
        "@blockchain_npm//truffle/bin:truffle",
    ],
    deps = [
        "//src/blockchain:deploy_ganache",
        "@bazel_tools//tools/bash/runfiles",
    ],
)

sh_binary(
    name = "run_adapter_server",
    srcs = [
        "run_adapter_server.sh",
    ],
    args = [
        "$(location //src/blockchain:truffle-config)",
        "$(location @blockchain_npm//truffle/bin:truffle)",
    ],
    data = [
        ":adapter_server_command",
        "//src/blockchain:compile_command",
        "//src/blockchain:truffle-config",
        "//src/blockchain/contracts",
        "//src/blockchain/migrations",
        "@blockchain_npm//truffle/bin:truffle",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@bazel_tools//tools/bash/runfiles",
    ],
)

command(
    name = "adapter_server_command",
    args = ["$$@"],
    command = ":adapter_server",
    visibility = ["//visibility:public"],
)

command(
    name = "run_adapter_server_command",
    args = [
        "$(location //src/blockchain:truffle-config)",
        "$(location @blockchain_npm//truffle/bin:truffle)",
    ],
    command = ":run_adapter_server",
    data = [
        ":adapter_server_command",
        "//src/blockchain:compile_command",
        "//src/blockchain:truffle-config",
        "//src/blockchain/contracts",
        "//src/blockchain/migrations",
        "@blockchain_npm//truffle/bin:truffle",
    ],
    visibility = ["//visibility:public"],
)
