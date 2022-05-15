load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

git_repository(
    name = "com_github_grpc_grpc",
    commit = "1c159689ceda2c408f7f9d97d96a264c9521b806",
    remote = "https://github.com/grpc/grpc",
)

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")

grpc_deps()

load("@com_github_grpc_grpc//bazel:grpc_extra_deps.bzl", "grpc_extra_deps")

grpc_extra_deps()

http_archive(
    name = "com_google_absl",
    strip_prefix = "abseil-cpp-215105818dfde3174fe799600bb0f3cae233d0bf",
    urls = ["https://github.com/abseil/abseil-cpp/archive/215105818dfde3174fe799600bb0f3cae233d0bf.zip"],
)

git_repository(
    name = "com_google_googletest",
    commit = "e2239ee6043f73722e7aa812a459f54a28552929",
    remote = "https://github.com/google/googletest",
)

git_repository(
    name = "protobuf_matchers",
    commit = "7c8e15741bcea83db7819cc472c3e96301a95158",
    remote = "https://github.com/inazarenko/protobuf-matchers",
)

git_repository(
    name = "hedron_compile_commands",
    commit = "0fe77b07dafed4b7172aced71108bc30d2a42ef5",
    remote = "https://github.com/hedronvision/bazel-compile-commands-extractor",
)

load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")

hedron_compile_commands_setup()
