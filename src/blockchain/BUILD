load("@build_bazel_rules_nodejs//:index.bzl", "npm_package_bin")

npm_package_bin(
    name = "compiled_blockchain",
    args = [
        "compile",
    ],
    chdir = package_name(),
    data = [
        ":truffle-config",
        "//src/blockchain/contracts",
        "//src/blockchain/migrations",
    ],
    output_dir = True,
    tool = "@blockchain_npm//truffle/bin:truffle",
)

filegroup(
    name = "truffle-config",
    srcs = ["truffle-config.js"],
)
