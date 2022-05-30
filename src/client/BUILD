cc_binary(
    name = "client_main",
    srcs = [
        "client_main.cc",
    ],
    deps = [
        ":client",
        "//src/proto:common",
        "//src/proto:coordinator",
        "@com_github_grpc_grpc//:grpc++",
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
        "//src/proto:common",
        "//src/proto:coordinator",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/types:variant",
        "@com_google_glog//:glog",
        "@thread_pool",
    ],
)
