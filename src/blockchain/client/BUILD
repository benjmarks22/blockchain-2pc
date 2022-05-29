load("@build_bazel_rules_nodejs//:index.bzl", "js_library")

js_library(
    name = "client",
    srcs = ["client.js"],
    deps = ["@blockchain_client_npm//web3-eth-contract"],
)

js_library(
    name = "main",
    srcs = ["main.js"],
    deps = [":client"],
)

sh_binary(
    name = "main_client",
    srcs = [
        "main_client.sh",
    ],
    args = [
        "$(location //src/blockchain:truffle-config)",
        "$(location @blockchain_npm//truffle/bin:truffle)",
        "$(location @blockchain_npm//ganache/bin:ganache)",
    ],
    data = [
        ":main",
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
