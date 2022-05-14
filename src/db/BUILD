cc_library(
    name = "data_source",
    hdrs = ["data_source.h"],
)

cc_binary(
    name = "lmdb_raw_lib_example_main",
    srcs = [
        "lmdb_raw_lib_example_main.cc",
    ],
    deps = [
        "@com_drycpp_lmdbxx//:lmdb++",
        "@com_google_absl//absl/flags:flag"
    ],
)
