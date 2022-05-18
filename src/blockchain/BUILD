sh_library(
    name = "setup_truffle",
    srcs = [
        "setup_truffle.sh",
    ],
    data = [
        ":truffle-config",
        "//src/blockchain/contracts",
        "//src/blockchain/migrations",
        "@blockchain_npm//truffle/bin:truffle",
    ],
    deps = [
        "@bazel_tools//tools/bash/runfiles",
    ],
)

sh_binary(
    name = "compile_blockchain",
    srcs = [
        "compile.sh",
    ],
    args = [
        "$(location :truffle-config)",
        "$(location @blockchain_npm//truffle/bin:truffle)",
    ],
    data = [
        ":truffle-config",
        "//src/blockchain/contracts",
        "//src/blockchain/migrations",
        "@blockchain_npm//truffle/bin:truffle",
    ],
    deps = [
        ":setup_truffle",
        "@bazel_tools//tools/bash/runfiles",
    ],
)

sh_binary(
    name = "deploy_truffle",
    srcs = [
        "deploy_truffle.sh",
    ],
    args = [
        "$(location :truffle-config)",
        "$(location @blockchain_npm//truffle/bin:truffle)",
    ],
    data = [
        ":truffle-config",
        "//src/blockchain/contracts",
        "//src/blockchain/migrations",
        "@blockchain_npm//truffle/bin:truffle",
    ],
    deps = [
        ":setup_truffle",
        "@bazel_tools//tools/bash/runfiles",
    ],
)

sh_binary(
    name = "deploy_ganache",
    srcs = [
        "deploy_ganache.sh",
    ],
    args = [
        "$(location :truffle-config)",
        "$(location @blockchain_npm//truffle/bin:truffle)",
        "$(location @blockchain_npm//ganache/bin:ganache)",
    ],
    data = [
        ":truffle-config",
        "//src/blockchain/contracts",
        "//src/blockchain/migrations",
        "@blockchain_npm//ganache/bin:ganache",
        "@blockchain_npm//truffle/bin:truffle",
    ],
    deps = [
        ":setup_truffle",
        "@bazel_tools//tools/bash/runfiles",
    ],
)

sh_test(
    name = "test_blockchain",
    srcs = [
        "run_tests.sh",
    ],
    args = [
        "$(location :truffle-config)",
        "$(location @blockchain_npm//truffle/bin:truffle)",
        "$(location @blockchain_npm//ganache/bin:ganache)",
    ],
    data = [
        ":truffle-config",
        "//src/blockchain/contracts",
        "//src/blockchain/migrations",
        "//src/blockchain/test",
        "@blockchain_npm//ganache/bin:ganache",
        "@blockchain_npm//truffle/bin:truffle",
    ],
    deps = [
        ":setup_truffle",
        "@bazel_tools//tools/bash/runfiles",
    ],
)

filegroup(
    name = "truffle-config",
    srcs = ["truffle-config.js"],
)
