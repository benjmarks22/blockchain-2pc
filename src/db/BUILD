cc_library(
    name = "database_transaction_adapter",
    hdrs = ["database_transaction_adapter.h"],
    deps = [
        "@com_google_absl//absl/status",
    ],
)

cc_library(
    name = "lmdb_database_transaction_adapter",
    srcs = [
        "lmdb_database_transaction_adapter.cc",
        "lmdb_database_transaction_adapter.h",
    ],
    hdrs = ["lmdb_database_transaction_adapter.h"],
    deps = [
        ":database_transaction_adapter",
        "@com_drycpp_lmdbxx//:lmdb++",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "lmdb_database_transaction_adapter_test",
    srcs = [
        "lmdb_database_transaction_adapter_test.cc",
    ],
    deps = [
        ":lmdb_database_transaction_adapter",
        "@com_google_absl//absl/status",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_binary(
    name = "lmdb_raw_lib_example_main",
    srcs = [
        "lmdb_raw_lib_example_main.cc",
    ],
    deps = [
        "@com_drycpp_lmdbxx//:lmdb++",
        "@com_google_absl//absl/flags:flag",
    ],
)
