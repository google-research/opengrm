![OpenGrm Logo](docs/images/logo.svg)

# OpenGrm Libraries

[![GitHub license](https://img.shields.io/badge/license-Apache2-blue.svg)](https://github.com/google-research/nisaba/blob/main/LICENSE)
[![C++ version](https://img.shields.io/badge/C++17-blue.svg?style=flat&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/17)
[![Bazel (x64 Linux)](https://github.com/google-research/opengrm/actions/workflows/bazel_x64_linux.yml/badge.svg)](https://github.com/google-research/opengrm/actions/workflows/bazel_x64_linux.yml)
[![Bazel (arm64 macOS)](https://github.com/google-research/opengrm/actions/workflows/bazel_arm64_macos.yml/badge.svg)](https://github.com/google-research/opengrm/actions/workflows/bazel_arm64_macos.yml)

OpenGrm is a collection of open-source libraries for constructing, combining,
applying and searching formal grammars and related representations, using the
[OpenFst](https://github.com/google-research/openfst) library for their
underlying finite-state models.

## Building

### Prerequisites

*   A C++17 compatible compiler such as
    [gcc >= 7.5.0 or clang >= 14.0.0](https://github.com/google/oss-policies-info/blob/main/foundational-cxx-support-matrix.md#compilers-tools-build-systems).

### Bazel

OpenFst can be built and tested using [Bazel](https://bazel.build) 8 or newer.

```bash
# Build the entire project
bazel build //...

# Run all tests
bazel test //...
```

Alternatively, [Bazelisk](https://github.com/bazelbuild/bazelisk) can be used
for building.

#### Example: Baum-Welch trainer

The following example builds Baum-Welch trainer using Bazelisk on macOS:

```shell
# Get OpenGrm and download Bazelisk.
BAZELISK_VERSION=...
git clone https://github.com/google-research/opengrm.git
wget https://github.com/bazelbuild/bazelisk/releases/download/v${BAZELISK_VERSION}/bazelisk-darwin-arm64
chmod +x bazelisk-darwin-arm64

# Build all the Baum-Welch tools and libraries, and run the tests.
cd opengrm
../bazelisk-darwin-arm64 build -c opt opengrm/baumwelch/...
../bazelisk-darwin-arm64 test -c opt opengrm/baumwelch/...

# Use the tool.
bazel-bin/opengrm/baumwelch/baumwelchtrain --help
```

### CMake

TODO: Complete and document CMake support.

## Pull Requests

At this time, we do not accept pull requests.

Commits may be force-pushed at any time until we start accepting them.

## License

OpenGrm is licensed under the terms of the Apache license. See
[LICENSE](LICENSE) for more information.

## Disclaimer

This is not an officially supported Google product. This project is not eligible
for the
[Google Open Source Software Vulnerability Rewards Program](https://bughunters.google.com/open-source-security).
