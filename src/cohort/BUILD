cc_binary(
    name = "cohort_server_main",
    srcs = [
        "cohort_server_main.cc",
    ],
    deps = [
        ":cohort_server",
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
        "//src/proto:cohort",
        "//src/utils:grpc_utils",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
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
