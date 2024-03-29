load("@com_github_ash2k_bazel_tools//multirun:def.bzl", "command")

cc_binary(
    name = "cohort_server_main",
    srcs = [
        "cohort_server_main.cc",
    ],
    deps = [
        ":cohort_server",
        "//src/blockchain:two_phase_commit",
        "//src/db:lmdb_database_transaction_adapter",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "cohort_server",
    srcs = [
        "cohort_server.cc",
        "cohort_server.h",
    ],
    hdrs = ["cohort_server.h"],
    deps = [
        "//src/blockchain:two_phase_commit",
        "//src/db:database_transaction_adapter",
        "//src/proto:cohort",
        "//src/utils:status_utils",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
        "@thread_pool",
    ],
)

cc_test(
    name = "cohort_server_test",
    srcs = [
        "cohort_server_test.cc",
    ],
    deps = [
        ":cohort_server",
        "//src/proto:cohort",
        "//src/proto:common",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@protobuf_matchers//protobuf-matchers",
    ],
)

sh_binary(
    name = "start_cohort_with_blockchain_adapter_server",
    srcs = [
        "start_cohort_with_blockchain_adapter_server.sh",
    ],
    args = [
        "$(location //src/blockchain:truffle-config)",
        "$(location @blockchain_npm//truffle/bin:truffle)",
    ],
    data = [
        ":cohort_server_main",
        "//src/blockchain:compile_command",
        "//src/blockchain:truffle-config",
        "//src/blockchain/client:adapter_server_command",
        "//src/blockchain/client:run_adapter_server_command",
        "@blockchain_npm//truffle/bin:truffle",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@bazel_tools//tools/bash/runfiles",
    ],
)

command(
    name = "start_cohort_with_blockchain_adapter_server_command",
    args = [
        "$(location //src/blockchain:truffle-config)",
        "$(location @blockchain_npm//truffle/bin:truffle)",
    ],
    command = ":start_cohort_with_blockchain_adapter_server",
    data = [
        ":cohort_server_main",
        "//src/blockchain:compile_command",
        "//src/blockchain:truffle-config",
        "//src/blockchain/client:adapter_server_command",
        "//src/blockchain/client:run_adapter_server_command",
        "@blockchain_npm//truffle/bin:truffle",
    ],
    visibility = ["//visibility:public"],
)
