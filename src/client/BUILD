cc_binary(
    name = "greeter_client_main",
    srcs = [
        "greeter_client_main.cc",
    ],
    deps = [
        ":greeter_client",
        "@com_github_grpc_grpc//:grpc++",
    ],
)

cc_library(
    name = "greeter_client",
    hdrs = ["greeter_client.h"],
    srcs = [
        "greeter_client.h",
        "greeter_client.cc",
    ],
    deps = [
        "@com_github_grpc_grpc//:grpc++",
        "//src/proto:greeter",
    ],
)