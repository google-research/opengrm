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

set(BAUMWELCH_HEADERS
    a-star.h
    baumwelch.h
    cascade.h
    decode.h
    expectation-table.h
    log-adder.h
    randomize.h
    score.h
    train.h
    util.h
)

grm_add_library(grm_baumwelch INTERFACE)

target_include_directories(
  grm_baumwelch INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(grm_baumwelch INTERFACE openfst::fst openfst::far
                                              grm_string
)

set(BAUMWELCHSCRIPT_SRCS
    decodescript.cc
    randomizescript.cc
    trainscript.cc
)

set(BAUMWELCHSCRIPT_HEADERS
    decodescript.h
    randomizescript.h
    trainscript.h
)

add_library(grm_baumwelchscript ${BAUMWELCHSCRIPT_SRCS})

target_include_directories(
  grm_baumwelchscript PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                             $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(grm_baumwelchscript
                      PUBLIC grm_baumwelch openfst::fstscript
                             absl::random_random
)

opengrm_install(
  TARGETS grm_baumwelchscript
  EXPORT OpenGrmTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

opengrm_install(FILES ${BAUMWELCH_HEADERS} ${BAUMWELCHSCRIPT_HEADERS}
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/opengrm/baumwelch
)

if(OPENGRM_ENABLE_BIN)
  add_executable(baumwelchrandomize baumwelchrandomize.cc
                                    baumwelchrandomize-main.cc
  )
  target_link_libraries(
    baumwelchrandomize
    PRIVATE grm_baumwelch
            grm_baumwelchscript
            openfst::fst
            absl::flags
  )
  opengrm_install(TARGETS baumwelchrandomize RUNTIME DESTINATION
                                                     ${CMAKE_INSTALL_BINDIR}
  )

  add_executable(baumwelchtrain baumwelchtrain.cc baumwelchtrain-main.cc)
  target_link_libraries(
    baumwelchtrain
    PRIVATE grm_baumwelch
            grm_baumwelchscript
            openfst::fst
            openfst::farscript
            absl::flags
  )
  opengrm_install(TARGETS baumwelchtrain RUNTIME DESTINATION
                                                 ${CMAKE_INSTALL_BINDIR}
  )

  add_executable(baumwelchdecode baumwelchdecode.cc baumwelchdecode-main.cc)
  target_link_libraries(
    baumwelchdecode
    PRIVATE grm_baumwelch
            grm_baumwelchscript
            openfst::fst
            openfst::farscript
            absl::flags
  )
  opengrm_install(TARGETS baumwelchdecode RUNTIME DESTINATION
                                                  ${CMAKE_INSTALL_BINDIR}
  )
endif()

if(OPENGRM_BUILD_TESTS)
  add_executable(a-star_test a-star_test.cc)
  target_link_libraries(a-star_test PRIVATE grm_baumwelch GTest::gtest_main)
  grm_configure_test_target(a-star_test)
  gtest_discover_tests(
    a-star_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  add_executable(log-adder_test log-adder_test.cc)
  target_link_libraries(log-adder_test PRIVATE grm_baumwelch GTest::gtest_main)
  grm_configure_test_target(log-adder_test)
  gtest_discover_tests(
    log-adder_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  # Build decipherment tests (always built if tests are enabled) mono-13
  add_executable(mono-13-log_test mono_test.cc)
  target_link_libraries(
    mono-13-log_test
    PRIVATE grm_baumwelch
            openfst::far
            openfst::phi-fst
            ${FORCE_LOAD_START}
            openfst::fst
            openfst::compact8_string-fst
            openfst::compact16_string-fst
            openfst::compact64_string-fst
            ${FORCE_LOAD_END}
            GTest::gtest_main
  )
  grm_configure_test_target(mono-13-log_test)
  target_compile_definitions(mono-13-log_test PRIVATE TEST_13 TEST_LOG)
  if(OPENGRM_RUN_SLOW_TESTS)
    gtest_discover_tests(
      mono-13-log_test
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
  endif()

  add_executable(mono-13-std_test mono_test.cc)
  target_link_libraries(
    mono-13-std_test
    PRIVATE grm_baumwelch
            openfst::far
            openfst::phi-fst
            ${FORCE_LOAD_START}
            openfst::fst
            openfst::compact8_string-fst
            openfst::compact16_string-fst
            openfst::compact64_string-fst
            ${FORCE_LOAD_END}
            GTest::gtest_main
  )
  grm_configure_test_target(mono-13-std_test)
  target_compile_definitions(mono-13-std_test PRIVATE TEST_13 TEST_STD)
  if(OPENGRM_RUN_SLOW_TESTS)
    gtest_discover_tests(
      mono-13-std_test
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
  endif()

  # mono-26
  add_executable(mono-26-log_test mono_test.cc)
  target_link_libraries(
    mono-26-log_test
    PRIVATE grm_baumwelch
            openfst::far
            openfst::phi-fst
            ${FORCE_LOAD_START}
            openfst::fst
            openfst::compact8_string-fst
            openfst::compact16_string-fst
            openfst::compact64_string-fst
            ${FORCE_LOAD_END}
            GTest::gtest_main
  )
  grm_configure_test_target(mono-26-log_test)
  target_compile_definitions(mono-26-log_test PRIVATE TEST_26 TEST_LOG)
  if(OPENGRM_RUN_SLOW_TESTS)
    gtest_discover_tests(
      mono-26-log_test
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
  endif()

  add_executable(mono-26-std_test mono_test.cc)
  target_link_libraries(
    mono-26-std_test
    PRIVATE grm_baumwelch
            openfst::far
            openfst::phi-fst
            ${FORCE_LOAD_START}
            openfst::fst
            openfst::compact8_string-fst
            openfst::compact16_string-fst
            openfst::compact64_string-fst
            ${FORCE_LOAD_END}
            GTest::gtest_main
  )
  grm_configure_test_target(mono-26-std_test)
  target_compile_definitions(mono-26-std_test PRIVATE TEST_26 TEST_STD)
  if(OPENGRM_RUN_SLOW_TESTS)
    gtest_discover_tests(
      mono-26-std_test
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
  endif()

  # mono-27
  add_executable(mono-27-log_test mono_test.cc)
  target_link_libraries(
    mono-27-log_test
    PRIVATE grm_baumwelch
            openfst::far
            openfst::phi-fst
            ${FORCE_LOAD_START}
            openfst::fst
            openfst::compact8_string-fst
            openfst::compact16_string-fst
            openfst::compact64_string-fst
            ${FORCE_LOAD_END}
            GTest::gtest_main
  )
  grm_configure_test_target(mono-27-log_test)
  target_compile_definitions(mono-27-log_test PRIVATE TEST_27 TEST_LOG)
  if(OPENGRM_RUN_SLOW_TESTS)
    gtest_discover_tests(
      mono-27-log_test
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
  endif()

  add_executable(mono-27-std_test mono_test.cc)
  target_link_libraries(
    mono-27-std_test
    PRIVATE grm_baumwelch
            openfst::far
            openfst::phi-fst
            ${FORCE_LOAD_START}
            openfst::fst
            openfst::compact8_string-fst
            openfst::compact16_string-fst
            openfst::compact64_string-fst
            ${FORCE_LOAD_END}
            GTest::gtest_main
  )
  grm_configure_test_target(mono-27-std_test)
  target_compile_definitions(mono-27-std_test PRIVATE TEST_27 TEST_STD)
  if(OPENGRM_RUN_SLOW_TESTS)
    gtest_discover_tests(
      mono-27-std_test
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
  endif()

  # z408
  add_executable(z408-log_test z408_test.cc)
  target_link_libraries(
    z408-log_test
    PRIVATE grm_baumwelch
            openfst::far
            openfst::phi-fst
            ${FORCE_LOAD_START}
            openfst::fst
            openfst::compact8_string-fst
            openfst::compact16_string-fst
            openfst::compact64_string-fst
            ${FORCE_LOAD_END}
            GTest::gtest_main
  )
  grm_configure_test_target(z408-log_test)
  target_compile_definitions(z408-log_test PRIVATE TEST_LOG)
  if(OPENGRM_RUN_SLOW_TESTS)
    gtest_discover_tests(
      z408-log_test
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
  endif()

  add_executable(z408-std_test z408_test.cc)
  target_link_libraries(
    z408-std_test
    PRIVATE grm_baumwelch
            openfst::far
            openfst::phi-fst
            ${FORCE_LOAD_START}
            openfst::fst
            openfst::compact8_string-fst
            openfst::compact16_string-fst
            openfst::compact64_string-fst
            ${FORCE_LOAD_END}
            GTest::gtest_main
  )
  grm_configure_test_target(z408-std_test)
  target_compile_definitions(z408-std_test PRIVATE TEST_STD)
  if(OPENGRM_RUN_SLOW_TESTS)
    gtest_discover_tests(
      z408-std_test
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
  endif()
endif()
