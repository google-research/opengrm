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

file(
  GLOB
  WALKER_SRCS
  "*.cc"
  "util/*.cc"
  "util/function/*.cc"
)

# Exclude test files
list(
  FILTER
  WALKER_SRCS
  EXCLUDE
  REGEX
  "_test\\.cc$"
)

grm_add_library(thrax_walker ${WALKER_SRCS})

target_include_directories(thrax_walker PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(
  thrax_walker
  PUBLIC thrax_core
         openfst::fst
         openfst::far
         openfst::fstscript
         grm_operators # For cdrewrite, etc.
         grm_rewrite
         thrax_ast
         absl::strings
         absl::status
         absl::container
         absl::log
)

if(OPENGRM_BUILD_TESTS)
  # walker/util tests
  add_executable(namespace_test util/namespace_test.cc)
  target_link_libraries(namespace_test PRIVATE thrax_walker GTest::gtest_main)
  grm_configure_test_target(namespace_test)
  gtest_discover_tests(
    namespace_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  add_executable(resource-map_test util/resource-map_test.cc)
  target_link_libraries(resource-map_test PRIVATE thrax_walker
                                                  GTest::gtest_main
  )
  grm_configure_test_target(resource-map_test)
  gtest_discover_tests(
    resource-map_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  # walker/util/function tests
  set(FUNCTION_TESTS
      assert-empty
      assert-equal
      assert-null
      closure
      compose
      concat
      determinize
      difference
      features
      lenientlycompose
      mpdtcompose
      pdtcompose
      project
      replace
      rewrite
      rmepsilon
      rmweight
      stringfile
      stringfst
      symboltable
      union
  )

  foreach(_test ${FUNCTION_TESTS})
    add_executable(function_${_test}_test util/function/${_test}_test.cc)
    target_link_libraries(
      function_${_test}_test
      PRIVATE ${FORCE_LOAD_START}
              thrax_walker
              openfst::fst
              openfst::far
              openfst::fstscript
              openfst::pdt
              openfst::mpdt
              ${FORCE_LOAD_END}
              GTest::gtest_main
    )
    grm_configure_test_target(function_${_test}_test)
    gtest_discover_tests(
      function_${_test}_test
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
  endforeach()
endif()
