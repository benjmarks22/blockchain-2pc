load("@com_github_grpc_grpc//bazel:grpc_build_system.bzl", "grpc_proto_library")

cc_binary(
    name = "client_main",
    srcs = [
        "client_main.cc",
    ],
    data = ["//src/client/requests"],
    deps = [
        ":client",
        "//src/proto:common",
        "//src/proto:coordinator",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "client",
    srcs = [
        "client.cc",
        "client.h",
    ],
    hdrs = ["client.h"],
    deps = [
        ":client_proto",
        "//src/proto:common",
        "//src/proto:coordinator",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/types:variant",
        "@com_google_glog//:glog",
        "@thread_pool",
    ],
)

grpc_proto_library(
    name = "client_proto",
    srcs = ["client.proto"],
    has_services = False,
    well_known_protos = True,
)

sh_binary(
    name = "end_to_end_test",
    srcs = [
        "end_to_end_test.sh",
    ],
    args = [
        "$(location //src/blockchain:truffle-config)",
        "$(location @blockchain_npm//truffle/bin:truffle)",
        "$(location @blockchain_npm//ganache/bin:ganache)",
    ],
    data = [
        ":client_main",
        "//src/blockchain:compile_command",
        "//src/blockchain:deploy_ganache_command",
        "//src/blockchain:truffle-config",
        "//src/blockchain/client:adapter_server_command",
        "//src/blockchain/client:run_adapter_server_command",
        "//src/blockchain/contracts",
        "//src/blockchain/migrations",
        "//src/cohort:start_cohort_with_blockchain_adapter_server",
        "//src/cohort:start_cohort_with_blockchain_adapter_server_command",
        "//src/coordinator:start_coordinator_with_blockchain_adapter_server",
        "//src/coordinator:start_coordinator_with_blockchain_adapter_server_command",
        "@blockchain_npm//ganache/bin:ganache",
        "@blockchain_npm//truffle/bin:truffle",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/blockchain:deploy_ganache",
        "@bazel_tools//tools/bash/runfiles",
    ],
)
