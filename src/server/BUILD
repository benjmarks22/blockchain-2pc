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
    srcs = [
        "greeter_server.cc",
        "greeter_server.h",
    ],
    hdrs = ["greeter_server.h"],
    deps = [
        "//src/proto:greeter",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "greeter_server_test",
    srcs = [
        "greeter_server_test.cc",
    ],
    deps = [
        ":greeter_server",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
