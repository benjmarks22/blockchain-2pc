cc_binary(
    name = "coordinator_server_main",
    srcs = [
        "coordinator_server_main.cc",
    ],
    deps = [
        ":coordinator_server",
        "//src/blockchain:two_phase_commit",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "coordinator_server",
    srcs = [
        "coordinator_server.cc",
        "coordinator_server.h",
    ],
    hdrs = ["coordinator_server.h"],
    deps = [
        "//src/blockchain:two_phase_commit",
        "//src/proto:cohort",
        "//src/proto:common",
        "//src/proto:coordinator",
        "//src/utils:status_utils",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@openssl",
        "@thread_pool",
    ],
)

cc_test(
    name = "coordinator_server_test",
    srcs = [
        "coordinator_server_test.cc",
    ],
    deps = [
        ":coordinator_server",
        "//src/blockchain/proto:two_phase_commit_adapter",
        "//src/proto:cohort",
        "//src/proto:common",
        "//src/proto:coordinator",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@protobuf_matchers//protobuf-matchers",
    ],
)
