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

set(SFST_HEADERS
    approx.h
    arpa.h
    backoff.h
    canonical.h
    count.h
    equal.h
    info.h
    intersect.h
    merge.h
    ngram-context.h
    ngram-count.h
    ngramapprox.h
    normalize.h
    perplexity.h
    phi2matcher.h
    randgen.h
    rmphi.h
    sfst.h
    sfstlib.h
    shortest-distance.h
    shrink.h
    smooth.h
    state-weights.h
    stationary-distrib.h
    topology.h
    trim.h
)

grm_add_library(grm_sfst ngram-context.cc stationary-distrib.cc)

target_include_directories(
  grm_sfst PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
  grm_sfst
  PUBLIC openfst::fst
         absl::check
         absl::core_headers
         absl::flat_hash_set
         absl::log
         absl::node_hash_map
         absl::node_hash_set
         absl::random_bit_gen_ref
         absl::span
         absl::strings
)

# ARPA reader/writer library
grm_add_library(grm_sfst_arpa arpa.cc)
target_include_directories(
  grm_sfst_arpa PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                       $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(
  grm_sfst_arpa
  PUBLIC openfst::fst
  PRIVATE grm_sfst
          absl::flat_hash_map
          absl::flat_hash_set
          absl::log
          absl::strings
)

opengrm_install(
  TARGETS grm_sfst grm_sfst_arpa
  EXPORT OpenGrmTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

opengrm_install(
  FILES ${SFST_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/opengrm/sfst
)

if(OPENGRM_ENABLE_BIN)
  set(SFST_TOOLS
      sfstapprox
      sfstcount
      sfstinfo
      sfstintersect
      sfstngramapprox
      sfstnormalize
      sfstperplexity
      sfstrandgen
      sfstshortestdistance
      sfsttopology
      sfsttrim
      sfstngramcount
      sfstngramsymbols
      sfstsmooth
      sfstshrink
      sfstmerge
      sfstngramread
      sfstngramprint
  )

  foreach(_tool ${SFST_TOOLS})
    add_executable(${_tool} ${_tool}.cc ${_tool}-main.cc)
    # Force-link openfst::fst so static builds retain the FST type registrations
    # in fst-types.cc needed by Fst::Read.
    target_link_libraries(
      ${_tool}
      PRIVATE grm_sfst
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
  # Extra link libraries for tools needing FAR or ARPA support
  target_link_libraries(sfstperplexity PRIVATE openfst::far)
  target_link_libraries(sfstngramcount PRIVATE openfst::far)
  target_link_libraries(sfstngramread PRIVATE grm_sfst_arpa)
  target_link_libraries(sfstngramprint PRIVATE grm_sfst_arpa)
endif()

if(OPENGRM_BUILD_TESTS)
  set(SFST_TESTS
      approx
      backoff
      canonical
      count
      equal
      ngram-context
      ngram-count
      ngramapprox
      normalize
      perplexity
      phi2matcher
      intersect
      randgen
      rmphi
      shortest-distance
      state-weights
      stationary-distrib
      topology
      trim
      sfstinfo
      arpa
      sfst
      smooth
      shrink
      merge
  )
  if(NOT OPENGRM_USE_SYSTEM_OPENFST)
    # algo_test.cc includes openfst/test/rand-fst.h, which OpenFst does not
    # install.
    list(PREPEND SFST_TESTS algo)
  endif()

  foreach(_test ${SFST_TESTS})
    add_executable(sfst_${_test}_test ${_test}_test.cc)
    target_link_libraries(
      sfst_${_test}_test
      PRIVATE grm_sfst
              ${OPENGRM_ALWAYS_LINK_START}
              openfst::fst
              ${OPENGRM_ALWAYS_LINK_END}
              GTest::gmock
              GTest::gtest_main
              absl::flags
    )
    grm_configure_test_target(sfst_${_test}_test)
    if(_test STREQUAL "algo")
      # algo_test.cc defines its own main() without GoogleTest TEST macros or
      # RUN_ALL_TESTS(), so gtest_discover_tests would execute the trials during
      # discovery and register 0 CTest tests.
      add_test(
        NAME sfst_algo_test
        COMMAND sfst_algo_test
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      )
    else()
      gtest_discover_tests(
        sfst_${_test}_test
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      )
    endif()
  endforeach()

  # Specific test dependencies
  if(NOT OPENGRM_USE_SYSTEM_OPENFST)
    target_link_libraries(sfst_algo_test PRIVATE absl::random_random)
  endif()
  target_link_libraries(sfst_arpa_test PRIVATE grm_sfst_arpa)
  target_link_libraries(
    sfst_intersect_test
    PRIVATE absl::flags_reflection
            absl::log_globals
            absl::log_severity
            absl::memory
  )
  foreach(
    _log_test
    canonical
    equal
    rmphi
    state-weights
    stationary-distrib
  )
    target_link_libraries(sfst_${_log_test}_test PRIVATE absl::log_globals
                                                         absl::log_severity
    )
  endforeach()
  target_link_libraries(sfst_perplexity_test PRIVATE openfst::far)
  target_link_libraries(sfst_shortest-distance_test PRIVATE openfst::fstscript)
  target_link_libraries(sfst_merge_test PRIVATE grm_sfst_arpa)

  if(OPENGRM_ENABLE_BIN)
    # Verify that command-line tools retain FST type registrations in static
    # builds.
    add_test(NAME sfstinfo_smoke_test
             COMMAND sfstinfo ${CMAKE_CURRENT_SOURCE_DIR}/testdata/earnest.mod
    )
  endif()
endif()
