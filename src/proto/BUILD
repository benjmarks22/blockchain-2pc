load("@com_github_grpc_grpc//bazel:grpc_build_system.bzl", "grpc_proto_library")

package(default_visibility = ["//visibility:public"])

grpc_proto_library(
    name = "coordinator",
    srcs = ["coordinator.proto"],
    deps = [
        ":common",
    ],
)

grpc_proto_library(
    name = "cohort",
    srcs = ["cohort.proto"],
    generate_mocks = True,
    deps = [
        ":common",
    ],
)

grpc_proto_library(
    name = "common",
    srcs = ["common.proto"],
    has_services = False,
    well_known_protos = True,
)
