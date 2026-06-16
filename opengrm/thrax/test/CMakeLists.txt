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

find_package(Threads REQUIRED)

add_library(thrax_total-sort INTERFACE)
target_include_directories(
  thrax_total-sort INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(thrax_total-sort INTERFACE openfst::fst)

# Only include tests that don't depend on pre-compiled grammars. Other
# regression tests (e.g., basic-regression, advanced-regression) are excluded
# because they depend on Bazel build rules (grm_build_defs.bzl) to compile
# grammars to FAR files before running the tests.
# TODO: Emulate grm_build_defs.bzl in CMake to compile grammars and
# re-enable these tests.
set(THRAX_TESTS lexer)

foreach(_test ${THRAX_TESTS})
  add_executable(thrax_${_test}_test ${_test}_test.cc)
  target_link_libraries(
    thrax_${_test}_test
    PRIVATE thrax_total-sort
            ${FORCE_LOAD_START}
            thrax_compiler_lib
            thrax_walker
            openfst::fst
            openfst::far
            openfst::fstscript
            openfst::pdt
            openfst::mpdt
            ${FORCE_LOAD_END}
            GTest::gmock_main
            absl::strings
            absl::log
            # It also used //thread for grm-manager_test
            Threads::Threads
  )
  grm_configure_test_target(thrax_${_test}_test)
  gtest_discover_tests(
    thrax_${_test}_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )
endforeach()
