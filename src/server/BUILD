cc_binary(
    name = "greeter_server_main",
    srcs = [
        "greeter_server_main.cc",
    ],
    deps = [
        ":greeter_server",
        "@com_github_grpc_grpc//:grpc++",
    ],
)

cc_library(
    name = "greeter_server",
    hdrs = ["greeter_server.h"],
    srcs = [
        "greeter_server.h",
        "greeter_server.cc",
    ],
    deps = [
        "@com_google_absl//absl/strings",
        "@com_github_grpc_grpc//:grpc++",
        "//src/proto:greeter",
    ],
)