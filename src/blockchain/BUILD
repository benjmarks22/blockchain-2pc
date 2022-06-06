load("@com_github_ash2k_bazel_tools//multirun:def.bzl", "command")

package(default_visibility = ["//src/blockchain:__subpackages__"])

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

command(
    name = "compile_command",
    args = [
        "$(location :truffle-config)",
        "$(location @blockchain_npm//truffle/bin:truffle)",
    ],
    command = "compile_blockchain",
    data = [
        ":truffle-config",
        "//src/blockchain/contracts",
        "//src/blockchain/migrations",
        "@blockchain_npm//truffle/bin:truffle",
    ],
    visibility = ["//visibility:public"],
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

sh_library(
    name = "deploy_ganache",
    srcs = [
        "deploy_ganache.sh",
    ],
    data = [
        ":truffle-config",
        "//src/blockchain/contracts",
        "//src/blockchain/migrations",
        "@blockchain_npm//ganache/bin:ganache",
        "@blockchain_npm//truffle/bin:truffle",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":setup_truffle",
        "@bazel_tools//tools/bash/runfiles",
    ],
)

command(
    name = "deploy_ganache_command",
    args = [
        "$(location :truffle-config)",
        "$(location @blockchain_npm//truffle/bin:truffle)",
        "$(location @blockchain_npm//ganache/bin:ganache)",
    ],
    command = ":deploy_ganache_without_console",
    data = [
        ":truffle-config",
        "//src/blockchain/contracts",
        "//src/blockchain/migrations",
        "@blockchain_npm//ganache/bin:ganache",
        "@blockchain_npm//truffle/bin:truffle",
    ],
    visibility = ["//visibility:public"],
)

sh_binary(
    name = "deploy_ganache_without_console",
    srcs = [
        "deploy_ganache_without_console.sh",
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
        ":deploy_ganache",
        "@bazel_tools//tools/bash/runfiles",
    ],
)

sh_binary(
    name = "deploy_ganache_with_console",
    srcs = [
        "deploy_ganache_with_console.sh",
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
        ":deploy_ganache",
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
    visibility = ["//visibility:public"],
)

cc_library(
    name = "two_phase_commit",
    srcs = [
        "two_phase_commit.cc",
        "two_phase_commit.h",
    ],
    hdrs = ["two_phase_commit.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//src/blockchain/proto:two_phase_commit_adapter",
        "//src/utils:status_utils",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
    ],
)

cc_test(
    name = "two_phase_commit_test",
    srcs = [
        "two_phase_commit_test.cc",
    ],
    deps = [
        ":two_phase_commit",
        "//src/blockchain/proto:two_phase_commit_adapter",
        "@com_google_absl//absl/status",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
