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

set(WALKER_SRCS
    evaluator.cc
    identifier-counter.cc
    printer.cc
    rule-dependency-extractor.cc
    walker.cc
    util/namespace.cc
    util/function/assert-empty.cc
    util/function/assert-equal.cc
    util/function/assert-null.cc
    util/function/cdrewrite.cc
    util/function/closure.cc
    util/function/compose.cc
    util/function/concat.cc
    util/function/determinize.cc
    util/function/difference.cc
    util/function/features.cc
    util/function/invert.cc
    util/function/lenientlycompose.cc
    util/function/loadfst.cc
    util/function/loadfstfromfar.cc
    util/function/minimize.cc
    util/function/mpdtcompose.cc
    util/function/optimize.cc
    util/function/paradigm.cc
    util/function/pdtcompose.cc
    util/function/project.cc
    util/function/replace.cc
    util/function/reverse.cc
    util/function/rewrite.cc
    util/function/rmepsilon.cc
    util/function/rmweight.cc
    util/function/stringfile.cc
    util/function/stringfst.cc
    util/function/symbols.cc
    util/function/symboltable.cc
    util/function/union.cc
)

set(WALKER_HEADERS
    evaluator.h
    identifier-counter.h
    printer.h
    rule-dependency-extractor.h
    walker.h
)

set(WALKER_UTIL_HEADERS
    util/datatype.h
    util/namespace.h
    util/register.h
    util/resource-map.h
)

set(WALKER_FUNCTION_HEADERS
    util/function/assert-empty.h
    util/function/assert-equal.h
    util/function/assert-null.h
    util/function/cdrewrite.h
    util/function/closure.h
    util/function/compose.h
    util/function/concat.h
    util/function/determinize.h
    util/function/difference.h
    util/function/features.h
    util/function/function.h
    util/function/invert.h
    util/function/lenientlycompose.h
    util/function/loadfst.h
    util/function/loadfstfromfar.h
    util/function/minimize.h
    util/function/mpdtcompose.h
    util/function/optimize.h
    util/function/paradigm.h
    util/function/pdtcompose.h
    util/function/project.h
    util/function/replace.h
    util/function/reverse.h
    util/function/rewrite.h
    util/function/rmepsilon.h
    util/function/rmweight.h
    util/function/stringfile.h
    util/function/stringfst.h
    util/function/symbols.h
    util/function/symboltable.h
    util/function/union.h
)

grm_add_library(thrax_walker ${WALKER_SRCS})

target_link_libraries(
  thrax_walker
  PUBLIC thrax_core
         openfst::fst
         openfst::far
         openfst::fstscript
         grm_operators # For cdrewrite, etc.
         grm_rewrite
         thrax_ast
         absl::btree
         absl::check
         absl::flags
         absl::flat_hash_map
         absl::flat_hash_set
         absl::log
         absl::memory
         absl::node_hash_map
         absl::node_hash_set
         absl::status
         absl::statusor
         absl::strings
         absl::synchronization
         absl::vlog_is_on
)

opengrm_install(
  TARGETS thrax_walker
  EXPORT OpenGrmTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

opengrm_install(
  FILES ${WALKER_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/opengrm/thrax/walker
)

opengrm_install(
  FILES ${WALKER_UTIL_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/opengrm/thrax/walker/util
)

opengrm_install(
  FILES ${WALKER_FUNCTION_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/opengrm/thrax/walker/util/function
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
      cdrewrite
      closure
      compose
      concat
      determinize
      difference
      features
      invert
      lenientlycompose
      loadfst
      loadfstfromfar
      minimize
      mpdtcompose
      optimize
      pdtcompose
      project
      replace
      reverse
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
      PRIVATE ${OPENGRM_ALWAYS_LINK_START}
              thrax_walker
              openfst::fst
              openfst::far
              openfst::fstscript
              openfst::pdt
              openfst::mpdt
              ${OPENGRM_ALWAYS_LINK_END}
              GTest::gtest_main
    )
    grm_configure_test_target(function_${_test}_test)
    gtest_discover_tests(
      function_${_test}_test
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
  endforeach()
endif()
