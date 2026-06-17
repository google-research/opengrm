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

set(STRING_SRCS
    defaults.cc
    stringcompile.cc
    stringfile.cc
    stringutil.cc
)

set(STRING_HEADERS
    defaults.h
    prefix_tree.h
    stringcompile.h
    stringfile.h
    stringmap.h
    stringprint.h
    stringutil.h
    string-view-fst.h
)

grm_add_library(grm_string ${STRING_SRCS})

target_include_directories(
  grm_string PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
  grm_string
  PUBLIC openfst::fst
         absl::strings
         absl::log
         absl::container # For btree in prefix_tree
)

set(STRINGSCRIPT_SRCS
    stringcompilescript.cc
    stringmapscript.cc
    stringprintscript.cc
)

set(STRINGSCRIPT_HEADERS
    stringcompilescript.h
    stringmapscript.h
    stringprintscript.h
)

add_library(grm_stringscript ${STRINGSCRIPT_SRCS})

target_include_directories(
  grm_stringscript PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(grm_stringscript PUBLIC grm_string openfst::fstscript)

opengrm_install(
  TARGETS
  grm_string
  grm_stringscript
  EXPORT
  OpenGrmTargets
  LIBRARY
  DESTINATION
  ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE
  DESTINATION
  ${CMAKE_INSTALL_LIBDIR}
)

opengrm_install(
  FILES
  ${STRING_HEADERS}
  ${STRINGSCRIPT_HEADERS}
  DESTINATION
  ${CMAKE_INSTALL_INCLUDEDIR}/opengrm/string
)

if(OPENGRM_BUILD_TESTS)
  # Just adding one test for illustration, we can add all
  add_executable(prefix_tree_test prefix_tree_test.cc)
  target_link_libraries(prefix_tree_test PRIVATE grm_string GTest::gtest_main)
  grm_configure_test_target(prefix_tree_test)
  gtest_discover_tests(
    prefix_tree_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  add_executable(stringcompile_test stringcompile_test.cc)
  target_link_libraries(stringcompile_test PRIVATE grm_string GTest::gmock_main)
  grm_configure_test_target(stringcompile_test)
  gtest_discover_tests(
    stringcompile_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  add_executable(stringutil_test stringutil_test.cc)
  target_link_libraries(stringutil_test PRIVATE grm_string GTest::gtest_main)
  grm_configure_test_target(stringutil_test)
  gtest_discover_tests(
    stringutil_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  add_executable(string-view-fst_test string-view-fst_test.cc)
  target_link_libraries(string-view-fst_test PRIVATE grm_string
                                                     GTest::gtest_main
  )
  grm_configure_test_target(string-view-fst_test)
  gtest_discover_tests(
    string-view-fst_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  # Tests with data
  add_executable(stringfile_test stringfile_test.cc)
  target_link_libraries(stringfile_test PRIVATE grm_string GTest::gtest_main)
  grm_configure_test_target(stringfile_test)
  gtest_discover_tests(
    stringfile_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  add_executable(stringmap_test stringmap_test.cc)
  target_link_libraries(stringmap_test PRIVATE grm_string grm_stringscript
                                               GTest::gtest_main
  )
  grm_configure_test_target(stringmap_test)
  gtest_discover_tests(
    stringmap_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  add_executable(string_test string_test.cc)
  target_link_libraries(string_test PRIVATE grm_string grm_stringscript
                                            GTest::gtest_main
  )
  grm_configure_test_target(string_test)
  # Uses add_test instead of gtest_discover_tests because the tests in
  # string_test.cc are order-dependent and rely on shared global compiler state
  # (the generated symbols table) populated by previous tests. Discovery
  # isolates each test case, which breaks these dependencies.
  add_test(
    NAME string_test
    COMMAND string_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )
endif()
