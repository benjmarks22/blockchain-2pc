load("@com_github_grpc_grpc//bazel:grpc_build_system.bzl", "grpc_proto_library")

package(default_visibility = ["//visibility:public"])

grpc_proto_library(
    name = "two_phase_commit_adapter",
    srcs = ["two_phase_commit_adapter.proto"],
    generate_mocks = True,
    well_known_protos = True,
)

filegroup(
    name = "two_phase_commit_adapter_proto_file",
    srcs = ["two_phase_commit_adapter.proto"],
)
