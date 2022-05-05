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
    srcs = [
        "greeter_client.cc",
        "greeter_client.h",
    ],
    hdrs = ["greeter_client.h"],
    deps = [
        "//src/proto:greeter",
        "@com_github_grpc_grpc//:grpc++",
    ],
)
