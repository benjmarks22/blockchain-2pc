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
    name = "com_google_googletest",
    commit = "e2239ee6043f73722e7aa812a459f54a28552929",
    remote = "https://github.com/google/googletest",
)

http_archive(
    name = "build_bazel_rules_nodejs",
    sha256 = "e328cb2c9401be495fa7d79c306f5ee3040e8a03b2ebb79b022e15ca03770096",
    urls = ["https://github.com/bazelbuild/rules_nodejs/releases/download/5.4.2/rules_nodejs-5.4.2.tar.gz"],
)

http_archive(
    name = "com_github_gflags_gflags",
    sha256 = "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
    strip_prefix = "gflags-2.2.2",
    urls = ["https://github.com/gflags/gflags/archive/v2.2.2.tar.gz"],
)

http_archive(
    name = "com_google_glog",
    sha256 = "21bc744fb7f2fa701ee8db339ded7dce4f975d0d55837a97be7d46e8382dea5a",
    strip_prefix = "glog-0.5.0",
    urls = ["https://github.com/google/glog/archive/v0.5.0.zip"],
)

load("@build_bazel_rules_nodejs//:repositories.bzl", "build_bazel_rules_nodejs_dependencies")

build_bazel_rules_nodejs_dependencies()

# The yarn_install rule runs yarn anytime the package.json or yarn.lock file changes.
# It also extracts and installs any Bazel rules distributed in an npm package.
load("@build_bazel_rules_nodejs//:index.bzl", "yarn_install")

yarn_install(
    name = "blockchain_npm",
    frozen_lockfile = False,
    package_json = "//src/blockchain:package.json",
    yarn_lock = "//src/blockchain:yarn.lock",
)

yarn_install(
    name = "blockchain_client_npm",
    frozen_lockfile = False,
    package_json = "//src/blockchain/client:package.json",
    yarn_lock = "//src/blockchain/client:yarn.lock",
)

new_git_repository(
    name = "openssl",
    build_file_content = """
cc_library(
    name = "openssl",
    srcs = glob([
        "include/openssl/*.h",
    ]),
    hdrs = glob([
        "include/openssl/*.h",
    ]),
    visibility = ["//visibility:public",],
)
""",
    commit = "4d346a188c27bdf78aa76590c641e1217732ca4b",
    remote = "https://github.com/openssl/openssl",
)

new_git_repository(
    name = "thread_pool",
    build_file_content = """
cc_library(
    name = "thread_pool",
    srcs = [
        "thread_pool.hpp",
    ],
    hdrs = [
        "thread_pool.hpp",
    ],
    visibility = ["//visibility:public",],
)
""",
    commit = "b6cd773f37b1be7718f771ae7403726a28be5f40",
    remote = "https://github.com/bshoshany/thread-pool",
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

load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")

hedron_compile_commands_setup()
