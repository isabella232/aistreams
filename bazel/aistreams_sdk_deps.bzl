"""Load dependencies needed to compile and test aistreams-sdk as a 3rd-party consumer."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def aistreams_sdk_deps():
    """Declare and load direct dependencies for aistreams-sdk."""

    maybe(
        http_archive,
        name = "com_github_grpc_grpc",
        strip_prefix = "grpc-1.29.1",
        urls = [
            "https://github.com/grpc/grpc/archive/v1.29.1.tar.gz",
        ],
        sha256 = "0343e6dbde66e9a31c691f2f61e98d79f3584e03a11511fad3f10e3667832a45",
    )

    maybe(
        http_archive,
        name = "com_google_absl",
        strip_prefix = "abseil-cpp-20190808",
        urls = [
            "https://github.com/abseil/abseil-cpp/archive/20190808.tar.gz",
        ],
        sha256 = "8100085dada279bf3ee00cd064d43b5f55e5d913be0dfe2906f06f8f28d5b37e",
    )

    maybe(
        http_archive,
        name = "com_github_google_glog",
        strip_prefix = "glog-0.4.0",
        urls = [
            "https://github.com/google/glog/archive/v0.4.0.tar.gz",
        ],
        sha256 = "f28359aeba12f30d73d9e4711ef356dc842886968112162bc73002645139c39c",
    )

    maybe(
        http_archive,
        name = "com_github_google_benchmark",
        urls = [
            "https://github.com/google/benchmark/archive/v1.5.1.tar.gz",
        ],
        strip_prefix = "benchmark-1.5.1",
        sha256 = "23082937d1663a53b90cb5b61df4bcc312f6dee7018da78ba00dd6bd669dfef2",
    )

    maybe(
        http_archive,
        name = "com_github_google_googletest",
        strip_prefix = "googletest-release-1.10.0",
        urls = [
            "https://github.com/google/googletest/archive/release-1.10.0.tar.gz",
        ],
        sha256 = "9dc9157a9a1551ec7a7e43daea9a694a0bb5fb8bec81235d8a1e6ef64c716dcb",
    )

    maybe(
        http_archive,
        name = "com_google_protobuf",
        sha256 = "416212e14481cff8fd4849b1c1c1200a7f34808a54377e22d7447efdf54ad758",
        strip_prefix = "protobuf-09745575a923640154bcf307fba8aedff47f240a",
        url = "https://github.com/google/protobuf/archive/09745575a923640154bcf307fba8aedff47f240a.tar.gz",  # 2019-08-19
    )

    maybe(
        http_archive,
        name = "com_google_googleapis",
        sha256 = "2368a76f8f39582b45d119579f4c8674ddb119cd764f91b87383dac376087694",
        strip_prefix = "googleapis-369e0cd05493fc5fdd7be96c6dc3b141e9dd0c16",
        # 2020-09-10
        # TODO: update the version once aistreams is published.
        url = "https://github.com/googleapis/googleapis/archive/369e0cd05493fc5fdd7be96c6dc3b141e9dd0c16.tar.gz",
    )

    # This requires gstreamer to be installed on your system.
    # TODO: Ok to start, but consider building from source with bazel.
    maybe(
        native.new_local_repository,
        name = "gstreamer",
        build_file = "//third_party:gstreamer.BUILD",
        path = "/usr",
    )

    maybe(
        http_archive,
        name = "pybind11",
        build_file = "@pybind11_bazel//:pybind11.BUILD",
        strip_prefix = "pybind11-2.2.3",
        urls = ["https://github.com/pybind/pybind11/archive/v2.2.3.tar.gz"],
        sha256 = "3a3b7b651afab1c5ba557f4c37d785a522b8030dfc765da26adc2ecd1de940ea",
    )

    maybe(
        http_archive,
        name = "pybind11_bazel",
        strip_prefix = "pybind11_bazel-26973c0ff320cb4b39e45bc3e4297b82bc3a6c09",
        urls = ["https://github.com/pybind/pybind11_bazel/archive/26973c0ff320cb4b39e45bc3e4297b82bc3a6c09.zip"],
        sha256 = "a5666d950c3344a8b0d3892a88dc6b55c8e0c78764f9294e806d69213c03f19d",
    )
