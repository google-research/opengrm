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

add_library(thrax_total-sort INTERFACE)
target_include_directories(thrax_total-sort
                           INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)
target_link_libraries(thrax_total-sort INTERFACE openfst::fst)

# Enable tests whose input grammars and checked-in FAR archives are available
# without compiling extra grammars at build time. Other regression tests (e.g.,
# basic-regression, advanced-regression) remain excluded because they depend on
# Bazel build rules (grm_build_defs.bzl) to compile grammars to FAR files before
# running the tests.
#
# TODO: Emulate grm_build_defs.bzl in CMake to compile grammars and
# re-enable the remaining regression tests.

add_executable(thrax_lexer_test lexer_test.cc)
target_link_libraries(
  thrax_lexer_test
  PRIVATE thrax_compiler_lib
          absl::check
          absl::log
          absl::strings
          GTest::gmock_main
)
grm_configure_test_target(thrax_lexer_test)
gtest_discover_tests(
  thrax_lexer_test
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_executable(thrax_grm-manager_test grm-manager_test.cc)
target_link_libraries(
  thrax_grm-manager_test
  PRIVATE ${OPENGRM_ALWAYS_LINK_START}
          openfst::fst
          ${OPENGRM_ALWAYS_LINK_END}
          thrax_core
          absl::log
          absl::strings
          GTest::gmock_main
)
grm_configure_test_target(thrax_grm-manager_test)
gtest_discover_tests(
  thrax_grm-manager_test
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_executable(thrax_rule-cascade_test rule-cascade_test.cc)
target_link_libraries(
  thrax_rule-cascade_test
  PRIVATE ${OPENGRM_ALWAYS_LINK_START}
          openfst::fst
          ${OPENGRM_ALWAYS_LINK_END}
          thrax_core
          absl::strings
          GTest::gtest_main
)
grm_configure_test_target(thrax_rule-cascade_test)
# Filter out PDT and MPDT tests that depend on Bazel-compiled FAR archives.
gtest_discover_tests(
  thrax_rule-cascade_test
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  TEST_FILTER "-RuleCascadeTest.*PDT*"
)

# Stage compilation testdata in the build tree so basic.far and advanced.far
# (required by import statements in advanced.grm and import.grm) can be copied
# from the checked-in .sttable.far archives without writing into the source
# tree.
set(_compilation_testdata_src
    "${CMAKE_CURRENT_SOURCE_DIR}/testdata/compilation"
)
set(_compilation_testdata_root
    "${CMAKE_CURRENT_BINARY_DIR}/compilation_testdata"
)
set(_compilation_testdata_dir
    "${_compilation_testdata_root}/opengrm/thrax/test/testdata/compilation"
)
set(_compilation_testdata_stamp "${_compilation_testdata_root}.stamp")
add_custom_command(
  OUTPUT "${_compilation_testdata_stamp}"
  BYPRODUCTS "${_compilation_testdata_dir}/basic.far"
             "${_compilation_testdata_dir}/advanced.far"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${_compilation_testdata_src}"
          "${_compilation_testdata_dir}"
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    "${_compilation_testdata_src}/basic.sttable.far"
    "${_compilation_testdata_dir}/basic.far"
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    "${_compilation_testdata_src}/advanced.sttable.far"
    "${_compilation_testdata_dir}/advanced.far"
  COMMAND ${CMAKE_COMMAND} -E touch "${_compilation_testdata_stamp}"
  DEPENDS "${_compilation_testdata_src}/advanced.grm"
          "${_compilation_testdata_src}/advanced.sttable.far"
          "${_compilation_testdata_src}/ast.grm"
          "${_compilation_testdata_src}/ast.txt"
          "${_compilation_testdata_src}/basic.grm"
          "${_compilation_testdata_src}/basic.sttable.far"
          "${_compilation_testdata_src}/import.grm"
          "${_compilation_testdata_src}/import.sttable.far"
          "${_compilation_testdata_src}/stringboundaries.grm"
          "${_compilation_testdata_src}/stringboundaries.sttable.far"
          "${_compilation_testdata_src}/symtab"
  COMMENT "Staging Thrax compilation_test testdata"
  VERBATIM
)
add_custom_target(thrax_compilation_testdata
                  DEPENDS "${_compilation_testdata_stamp}"
)

add_executable(thrax_compilation_test compilation_test.cc)
add_dependencies(thrax_compilation_test thrax_compilation_testdata)
target_link_libraries(
  thrax_compilation_test
  PRIVATE thrax_total-sort
          ${OPENGRM_ALWAYS_LINK_START}
          thrax_walker
          openfst::fst
          ${OPENGRM_ALWAYS_LINK_END}
          thrax_compiler_lib
          thrax_core
          thrax_ast
          openfst::far
          absl::check
          absl::strings
          GTest::gtest_main
)
grm_configure_test_target(thrax_compilation_test)
gtest_discover_tests(
  thrax_compilation_test
  WORKING_DIRECTORY "${_compilation_testdata_root}"
)
