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

set(REWRITE_HEADERS
    base_rule_cascade.h
    parentheses.h
    rewrite.h
    rewrite_manager.h
    rule_cascade.h
)

grm_add_library(grm_rewrite INTERFACE)

target_include_directories(
  grm_rewrite INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
  grm_rewrite
  INTERFACE openfst::fst
            openfst::far # Requires FAR extension
            grm_paths
            grm_string
            protobuf::libprotobuf
)

opengrm_install(
  FILES
  ${REWRITE_HEADERS}
  DESTINATION
  ${CMAKE_INSTALL_INCLUDEDIR}/opengrm/rewrite
)

if(OPENGRM_BUILD_TESTS)
  add_executable(base_rule_cascade_test base_rule_cascade_test.cc)
  target_link_libraries(
    base_rule_cascade_test
    PRIVATE ${OPENGRM_ALWAYS_LINK_START}
            grm_rewrite
            grm_operators
            openfst::fst
            openfst::far
            ${OPENGRM_ALWAYS_LINK_END}
            GTest::gmock_main
  ) # Wait, depends on operators:optimize
  # We'll need to make sure this links or only build if operators is available
  # (which it is, since it's core)
  grm_configure_test_target(base_rule_cascade_test)
  gtest_discover_tests(
    base_rule_cascade_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  add_executable(rewrite_test rewrite_test.cc)
  target_link_libraries(
    rewrite_test
    PRIVATE ${OPENGRM_ALWAYS_LINK_START}
            grm_rewrite
            openfst::fst
            openfst::far
            ${OPENGRM_ALWAYS_LINK_END}
            GTest::gmock_main
  )
  grm_configure_test_target(rewrite_test)
  gtest_discover_tests(
    rewrite_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  add_executable(parentheses_test parentheses_test.cc)
  target_link_libraries(
    parentheses_test
    PRIVATE ${OPENGRM_ALWAYS_LINK_START}
            grm_rewrite
            openfst::fst
            ${OPENGRM_ALWAYS_LINK_END}
            GTest::gmock_main
  )
  grm_configure_test_target(parentheses_test)
  gtest_discover_tests(
    parentheses_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  # These tests require Thrax data
  if(OPENGRM_ENABLE_THRAX)
    add_executable(rewrite_manager_test rewrite_manager_test.cc)
    target_link_libraries(
      rewrite_manager_test
      PRIVATE ${OPENGRM_ALWAYS_LINK_START}
              grm_rewrite
              openfst::fst
              openfst::far
              ${OPENGRM_ALWAYS_LINK_END}
              GTest::gmock_main
    )
    grm_configure_test_target(rewrite_manager_test)
    gtest_discover_tests(
      rewrite_manager_test
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )

    add_executable(rule_cascade_test rule_cascade_test.cc)
    target_link_libraries(
      rule_cascade_test
      PRIVATE ${OPENGRM_ALWAYS_LINK_START}
              grm_rewrite
              openfst::fst
              openfst::far
              ${OPENGRM_ALWAYS_LINK_END}
              GTest::gmock_main
    )
    grm_configure_test_target(rule_cascade_test)
    # Use add_test instead of gtest_discover_tests to allow filtering out PDT
    # and MPDT tests which depend on generated Thrax data.
    add_test(NAME rule_cascade_test
             COMMAND rule_cascade_test
                     "--gtest_filter=-RuleCascadeTest.PDT:RuleCascadeTest.MPDT"
    )
    set_tests_properties(
      rule_cascade_test
      PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
  endif()
endif()
