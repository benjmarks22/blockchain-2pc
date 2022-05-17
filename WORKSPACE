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

http_archive(
    name = "build_bazel_rules_nodejs",
    sha256 = "e328cb2c9401be495fa7d79c306f5ee3040e8a03b2ebb79b022e15ca03770096",
    urls = ["https://github.com/bazelbuild/rules_nodejs/releases/download/5.4.2/rules_nodejs-5.4.2.tar.gz"],
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
