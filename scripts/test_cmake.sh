#!/bin/bash

# Copyright 2026 The OpenGrm Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -euxo pipefail

echo "--- Running OpenGrm CMake tests ---"

NPROC="$(getconf _NPROCESSORS_ONLN)"

# Enable all switches to ensure we test all components
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Dev \
  -DOPENGRM_ENABLE_BIN=ON \
  -DOPENGRM_BUILD_TESTS=ON \
  -DOPENGRM_ENABLE_SFST=ON \
  -DOPENGRM_ENABLE_NGRAM=ON \
  -DOPENGRM_ENABLE_BAUMWELCH=ON \
  -DOPENGRM_ENABLE_THRAX=ON \
  -DBUILD_SHARED_LIBS=ON

cmake --build build -j "$NPROC"
ctest --test-dir build --output-on-failure -j "$NPROC"
