cc_library(
    name = "data_source",
    hdrs = ["data_source.h"],
    deps = [
        "@com_google_absl//absl/status",
    ],
)

cc_library(
    name = "lmdb_data_source",
    srcs = [
        "lmdb_data_source.cc",
        "lmdb_data_source.h",
    ],
    hdrs = ["lmdb_data_source.h"],
    deps = [
        ":data_source",
        "@com_drycpp_lmdbxx//:lmdb++",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "lmdb_data_source_test",
    srcs = [
        "lmdb_data_source_test.cc",
    ],
    deps = [
        ":lmdb_data_source",
        "@com_google_absl//absl/status",
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
