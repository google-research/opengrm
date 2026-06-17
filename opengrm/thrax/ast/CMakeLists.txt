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

set(AST_SRCS
    collection-node.cc
    fst-node.cc
    function-node.cc
    grammar-node.cc
    identifier-node.cc
    import-node.cc
    node.cc
    return-node.cc
    rule-node.cc
    statement-node.cc
    string-node.cc
)

set(AST_HEADERS
    collection-node.h
    fst-node.h
    function-node.h
    grammar-node.h
    identifier-node.h
    import-node.h
    node.h
    return-node.h
    rule-node.h
    statement-node.h
    string-node.h
)

grm_add_library(thrax_ast ${AST_SRCS})

target_include_directories(thrax_ast PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(thrax_ast PUBLIC absl::strings absl::log)

opengrm_install(
  FILES
  ${AST_HEADERS}
  DESTINATION
  ${CMAKE_INSTALL_INCLUDEDIR}/opengrm/thrax/ast
)

if(OPENGRM_BUILD_TESTS)
  set(AST_TESTS
      collection-node
      fst-node
      function-node
      grammar-node
      identifier-node
      import-node
      return-node
      rule-node
      statement-node
      string-node
  )

  foreach(_test ${AST_TESTS})
    add_executable(${_test}_test ${_test}_test.cc)
    target_link_libraries(${_test}_test PRIVATE thrax_ast GTest::gtest_main)
    grm_configure_test_target(${_test}_test)
    gtest_discover_tests(
      ${_test}_test
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
  endforeach()
endif()
