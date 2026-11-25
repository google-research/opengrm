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

target_link_libraries(
  grm_baumwelch
  INTERFACE openfst::fst
            openfst::far
            grm_string
            absl::check
            absl::flat_hash_map
            absl::hash
            absl::log
            absl::random_bit_gen_ref
            absl::random_distributions
            absl::random_random
            absl::strings
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

grm_add_library(grm_baumwelchscript ${BAUMWELCHSCRIPT_SRCS})

target_link_libraries(
  grm_baumwelchscript
  PUBLIC grm_baumwelch
         openfst::fstscript
         openfst::farscript
         absl::random_bit_gen_ref
)

opengrm_install(
  TARGETS grm_baumwelch grm_baumwelchscript
  EXPORT OpenGrmTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

opengrm_install(
  FILES ${BAUMWELCH_HEADERS} ${BAUMWELCHSCRIPT_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/opengrm/baumwelch
)

if(OPENGRM_ENABLE_BIN)
  # Force-link openfst::fst so static builds retain the FST type registrations
  # in fst-types.cc needed by Fst::Read.
  add_executable(baumwelchrandomize baumwelchrandomize.cc
                                    baumwelchrandomize-main.cc
  )
  target_link_libraries(
    baumwelchrandomize
    PRIVATE grm_baumwelch
            grm_baumwelchscript
            ${OPENGRM_ALWAYS_LINK_START}
            openfst::fst
            ${OPENGRM_ALWAYS_LINK_END}
            absl::flags
  )
  opengrm_install(
    TARGETS baumwelchrandomize
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  add_executable(baumwelchtrain baumwelchtrain.cc baumwelchtrain-main.cc)
  target_link_libraries(
    baumwelchtrain
    PRIVATE grm_baumwelch
            grm_baumwelchscript
            ${OPENGRM_ALWAYS_LINK_START}
            openfst::fst
            ${OPENGRM_ALWAYS_LINK_END}
            openfst::farscript
            absl::flags
  )
  opengrm_install(
    TARGETS baumwelchtrain
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  add_executable(baumwelchdecode baumwelchdecode.cc baumwelchdecode-main.cc)
  target_link_libraries(
    baumwelchdecode
    PRIVATE grm_baumwelch
            grm_baumwelchscript
            ${OPENGRM_ALWAYS_LINK_START}
            openfst::fst
            ${OPENGRM_ALWAYS_LINK_END}
            openfst::farscript
            absl::flags
  )
  opengrm_install(
    TARGETS baumwelchdecode
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

if(OPENGRM_BUILD_TESTS)
  # a-star_test is prefixed with grm_ to avoid target collision with OpenFst's
  # a-star_test when OPENFST_BUILD_TESTS is enabled.
  foreach(
    _test
    a-star
    cascade
    decode
    expectation-table
    log-adder
    randomize
    score
    train
  )
    if(_test STREQUAL "a-star")
      set(_target grm_a-star_test)
    else()
      set(_target ${_test}_test)
    endif()
    add_executable(${_target} ${_test}_test.cc)
    # Force-link openfst::fst so FarReader-backed tests retain FST type
    # registrations in static builds.
    target_link_libraries(
      ${_target}
      PRIVATE grm_baumwelch
              ${OPENGRM_ALWAYS_LINK_START}
              openfst::fst
              ${OPENGRM_ALWAYS_LINK_END}
              GTest::gmock
              GTest::gtest_main
    )
    grm_configure_test_target(${_target})
    gtest_discover_tests(
      ${_target}
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
  endforeach()

  if(OPENGRM_ENABLE_BIN)
    # Verify that command-line tools retain FST type registrations in static
    # builds.
    add_test(
      NAME baumwelchrandomize_smoke_test
      COMMAND
        baumwelchrandomize ${CMAKE_CURRENT_SOURCE_DIR}/testdata/mono/lm.fst
        ${CMAKE_CURRENT_BINARY_DIR}/baumwelchrandomize_smoke.fst
    )
  endif()

  # Build decipherment tests (always built if tests are enabled).
  foreach(
    _cipher
    mono-13
    mono-26
    mono-27
    z408
  )
    if(_cipher STREQUAL "z408")
      set(_src z408_test.cc)
      set(_cipher_def "")
    else()
      set(_src mono_test.cc)
      string(
        REPLACE "mono-"
                "TEST_"
                _cipher_def
                "${_cipher}"
      )
    endif()
    foreach(_semiring log std)
      set(_target ${_cipher}-${_semiring}_test)
      string(TOUPPER "TEST_${_semiring}" _semiring_def)
      set(_phi_fst "")
      if(_semiring STREQUAL "log")
        set(_phi_fst openfst::phi-fst)
      endif()
      add_executable(${_target} ${_src})
      target_link_libraries(
        ${_target}
        PRIVATE grm_baumwelch
                openfst::far
                ${OPENGRM_ALWAYS_LINK_START}
                ${_phi_fst}
                openfst::compact8_string-fst
                openfst::compact16_string-fst
                openfst::compact64_string-fst
                openfst::fst
                ${OPENGRM_ALWAYS_LINK_END}
                GTest::gtest_main
      )
      grm_configure_test_target(${_target})
      target_compile_definitions(${_target} PRIVATE ${_cipher_def}
                                                    ${_semiring_def}
      )
      if(OPENGRM_RUN_SLOW_TESTS)
        gtest_discover_tests(
          ${_target}
          WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        )
      endif()
    endforeach()
  endforeach()
endif()
