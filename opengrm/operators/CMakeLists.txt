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

set(OPERATORS_HEADERS
    cdrewrite.h
    checkprops.h
    concatrange.h
    cross.h
    getters.h
    lenientlycompose.h
    optimize.h
)

grm_add_library(grm_operators getters.cc)

target_link_libraries(
  grm_operators
  PUBLIC openfst::fst
         absl::log
         absl::span
         absl::strings
)

set(OPERATORSSCRIPT_SRCS
    cdrewritescript.cc
    concatrangescript.cc
    crossscript.cc
    lenientlycomposescript.cc
    optimizescript.cc
)

set(OPERATORSSCRIPT_HEADERS
    cdrewritescript.h
    concatrangescript.h
    crossscript.h
    lenientlycomposescript.h
    optimizescript.h
)

grm_add_library(grm_operatorsscript ${OPERATORSSCRIPT_SRCS})

target_link_libraries(grm_operatorsscript PUBLIC grm_operators
                                                 openfst::fstscript
)

opengrm_install(
  TARGETS grm_operators grm_operatorsscript
  EXPORT OpenGrmTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

opengrm_install(
  FILES ${OPERATORS_HEADERS} ${OPERATORSSCRIPT_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/opengrm/operators
)

if(OPENGRM_ENABLE_BIN)
  set(OPERATORS_TOOLS
      fstcdrewrite
      fstconcatrange
      fstcross
      fstlenientlycompose
      fstoptimize
  )
  # Force-link openfst::fst so static builds retain the FST type registrations
  # in fst-types.cc needed by Fst::Read.
  foreach(_tool ${OPERATORS_TOOLS})
    add_executable(${_tool} ${_tool}.cc ${_tool}-main.cc)
    target_link_libraries(
      ${_tool}
      PRIVATE grm_operators
              grm_operatorsscript
              ${OPENGRM_ALWAYS_LINK_START}
              openfst::fst
              ${OPENGRM_ALWAYS_LINK_END}
              absl::flags
    )
    opengrm_install(
      TARGETS ${_tool}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
  endforeach()
endif()

if(OPENGRM_BUILD_TESTS)
  add_executable(checkprops_test checkprops_test.cc)
  target_link_libraries(checkprops_test PRIVATE grm_operators GTest::gtest_main)
  grm_configure_test_target(checkprops_test)
  gtest_discover_tests(
    checkprops_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  add_executable(cdrewrite_test cdrewrite_test.cc)
  target_link_libraries(cdrewrite_test
                        PRIVATE grm_operators grm_operatorsscript
                                GTest::gtest_main
  )
  grm_configure_test_target(cdrewrite_test)
  gtest_discover_tests(
    cdrewrite_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  add_executable(cross_test cross_test.cc)
  target_link_libraries(
    cross_test
    PRIVATE grm_operators
            grm_operatorsscript
            grm_stringscript
            GTest::gtest_main
  )
  grm_configure_test_target(cross_test)
  gtest_discover_tests(
    cross_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  add_executable(lenientlycompose_test lenientlycompose_test.cc)
  target_link_libraries(lenientlycompose_test
                        PRIVATE grm_operators grm_operatorsscript
                                GTest::gtest_main
  )
  grm_configure_test_target(lenientlycompose_test)
  gtest_discover_tests(
    lenientlycompose_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  add_executable(optimize_test optimize_test.cc)
  target_link_libraries(optimize_test PRIVATE grm_operators grm_operatorsscript
                                              GTest::gtest_main
  )
  grm_configure_test_target(optimize_test)
  gtest_discover_tests(
    optimize_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  add_executable(concatrange_test concatrange_test.cc)
  target_link_libraries(
    concatrange_test
    PRIVATE grm_operators
            grm_operatorsscript
            grm_rewrite
            absl::check
            GTest::gmock_main
  )
  grm_configure_test_target(concatrange_test)
  gtest_discover_tests(
    concatrange_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  if(OPENGRM_ENABLE_BIN)
    # Verify that command-line tools retain FST type registrations in static
    # builds.
    add_test(NAME fstoptimize_smoke_test
             COMMAND fstoptimize ${CMAKE_CURRENT_SOURCE_DIR}/testdata/a.fst
                     ${CMAKE_CURRENT_BINARY_DIR}/fstoptimize_smoke.fst
    )
  endif()
endif()
