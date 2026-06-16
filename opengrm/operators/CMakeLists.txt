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

target_include_directories(
  grm_operators PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                       $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(grm_operators PUBLIC openfst::fst grm_rewrite
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

add_library(grm_operatorsscript ${OPERATORSSCRIPT_SRCS})

target_include_directories(
  grm_operatorsscript PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                             $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(grm_operatorsscript PUBLIC grm_operators
                                                 openfst::fstscript
)

opengrm_install(
  TARGETS grm_operators grm_operatorsscript
  EXPORT OpenGrmTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

opengrm_install(FILES ${OPERATORS_HEADERS} ${OPERATORSSCRIPT_HEADERS}
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/opengrm/operators
)

if(OPENGRM_ENABLE_BIN)
  # fstcdrewrite
  add_executable(fstcdrewrite fstcdrewrite.cc fstcdrewrite-main.cc)
  target_link_libraries(
    fstcdrewrite
    PRIVATE grm_operators
            grm_operatorsscript
            openfst::fst
            absl::flags
  )
  opengrm_install(TARGETS fstcdrewrite RUNTIME DESTINATION
                                               ${CMAKE_INSTALL_BINDIR}
  )

  # fstcross
  add_executable(fstcross fstcross.cc fstcross-main.cc)
  target_link_libraries(
    fstcross
    PRIVATE grm_operators
            grm_operatorsscript
            openfst::fst
            absl::flags
  )
  opengrm_install(TARGETS fstcross RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

  # fstlenientlycompose
  add_executable(fstlenientlycompose fstlenientlycompose.cc
                                     fstlenientlycompose-main.cc
  )
  target_link_libraries(
    fstlenientlycompose
    PRIVATE grm_operators
            grm_operatorsscript
            openfst::fst
            absl::flags
  )
  opengrm_install(TARGETS fstlenientlycompose RUNTIME DESTINATION
                                                      ${CMAKE_INSTALL_BINDIR}
  )

  # fstoptimize
  add_executable(fstoptimize fstoptimize.cc fstoptimize-main.cc)
  target_link_libraries(
    fstoptimize
    PRIVATE grm_operators
            grm_operatorsscript
            openfst::fst
            absl::flags
  )
  opengrm_install(TARGETS fstoptimize RUNTIME DESTINATION
                                              ${CMAKE_INSTALL_BINDIR}
  )

  # fstconcatrange
  add_executable(fstconcatrange fstconcatrange.cc fstconcatrange-main.cc)
  target_link_libraries(
    fstconcatrange
    PRIVATE grm_operators
            grm_operatorsscript
            openfst::fst
            absl::flags
  )
  opengrm_install(TARGETS fstconcatrange RUNTIME DESTINATION
                                                 ${CMAKE_INSTALL_BINDIR}
  )
endif()

if(OPENGRM_BUILD_TESTS)
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
  target_link_libraries(concatrange_test
                        PRIVATE grm_operators grm_operatorsscript
                                GTest::gmock_main
  )
  grm_configure_test_target(concatrange_test)
  gtest_discover_tests(
    concatrange_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  # We might also want to add the shell tests or just mention them
endif()
