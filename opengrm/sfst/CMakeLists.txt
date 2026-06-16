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

grm_add_library(grm_sfst stationary-distrib.cc)

target_include_directories(
  grm_sfst PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
  grm_sfst
  PUBLIC openfst::fst
         absl::strings
         absl::log
         absl::container
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
         absl::strings
         absl::container
)

# Merge interface library
grm_add_library(grm_sfst_merge INTERFACE)
target_include_directories(
  grm_sfst_merge INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(
  grm_sfst_merge INTERFACE grm_sfst openfst::fst
)

# Smooth interface library
grm_add_library(grm_sfst_smooth INTERFACE)
target_include_directories(
  grm_sfst_smooth INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(
  grm_sfst_smooth INTERFACE grm_sfst openfst::fst absl::log
)

# Shrink interface library
grm_add_library(grm_sfst_shrink INTERFACE)
target_include_directories(
  grm_sfst_shrink INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(
  grm_sfst_shrink INTERFACE grm_sfst openfst::fst absl::log absl::strings
)

opengrm_install(
  TARGETS grm_sfst
          grm_sfst_arpa
          grm_sfst_merge
          grm_sfst_smooth
          grm_sfst_shrink
  EXPORT OpenGrmTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

opengrm_install(FILES ${SFST_HEADERS}
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
    target_link_libraries(${_tool} PRIVATE grm_sfst openfst::fst absl::flags)
    opengrm_install(TARGETS ${_tool} RUNTIME DESTINATION
                                             ${CMAKE_INSTALL_BINDIR}
    )
  endforeach()
  # sfstperplexity requires the OpenFst FAR extension
  target_link_libraries(sfstperplexity PRIVATE openfst::far)

  # Extra link libraries for new tools
  target_link_libraries(sfstngramcount PRIVATE openfst::far)
  target_link_libraries(sfstngramread PRIVATE grm_sfst_arpa)
  target_link_libraries(sfstngramprint PRIVATE grm_sfst_arpa)
  target_link_libraries(sfstsmooth PRIVATE grm_sfst_smooth)
  target_link_libraries(sfstshrink PRIVATE grm_sfst_shrink)
  target_link_libraries(sfstmerge PRIVATE grm_sfst_merge)
endif()

if(OPENGRM_BUILD_TESTS)
  set(SFST_TESTS
      algo
      approx
      backoff
      count
      ngramapprox
      normalize
      perplexity
      phi2matcher
      randgen
      shortest-distance
      topology
      trim
      sfstinfo
      arpa
      smooth
      shrink
      merge
  )

  foreach(_test ${SFST_TESTS})
    add_executable(sfst_${_test}_test ${_test}_test.cc)
    target_link_libraries(
      sfst_${_test}_test
      PRIVATE grm_sfst
              openfst::far
              ${FORCE_LOAD_START}
              openfst::fst
              ${FORCE_LOAD_END}
              GTest::gtest_main
    ) # Perplexity/merge need far
    grm_configure_test_target(sfst_${_test}_test)
    gtest_discover_tests(
      sfst_${_test}_test
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
  endforeach()

  # Specific test dependencies
  target_link_libraries(sfst_arpa_test PRIVATE grm_sfst_arpa)
  target_link_libraries(sfst_smooth_test PRIVATE grm_sfst_smooth)
  target_link_libraries(sfst_shrink_test PRIVATE grm_sfst_shrink grm_sfst_smooth)
  target_link_libraries(sfst_merge_test PRIVATE grm_sfst_arpa grm_sfst_merge)
endif()
