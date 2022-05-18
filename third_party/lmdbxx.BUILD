# Description:
#   C++ LMDB Wrapper
cc_library(
    name = "lmdb++",
    hdrs = [
        "lmdb++.h",
    ],
    include_prefix = "lmdbxx",
    deps = ["@lmdb//:lmdb"],
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
)