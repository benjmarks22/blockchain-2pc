load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")
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
    name = "googletest",
    commit = "e2239ee6043f73722e7aa812a459f54a28552929",
    remote = "https://github.com/google/googletest",
)

git_repository(
    name = "hedron_compile_commands",
    commit = "0fe77b07dafed4b7172aced71108bc30d2a42ef5",
    remote = "https://github.com/hedronvision/bazel-compile-commands-extractor",
)

load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")

hedron_compile_commands_setup()

http_archive(
    name = "lmdb", 
    build_file = "//third_party:lmdb.BUILD",
    # From https://github.com/LMDB/lmdb/tags
    strip_prefix = "lmdb-LMDB_0.9.29/libraries/liblmdb",
    urls = ["https://github.com/LMDB/lmdb/archive/LMDB_0.9.29.tar.gz"],
)

new_git_repository(
    name = "com_drycpp_lmdbxx",
    branch = "master",
    build_file = "//third_party:lmdbxx.BUILD",
    remote = "https://github.com/drycpp/lmdbxx.git",
)