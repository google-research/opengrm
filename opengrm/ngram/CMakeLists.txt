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

set(NGRAM_SRCS
    ngram-absolute.cc
    ngram-context.cc
    ngram-count.cc
    ngram-count-prune.cc
    ngram-input.cc
    ngram-kneser-ney.cc
    ngram-list-prune.cc
    ngram-make.cc
    ngram-marginalize.cc
    ngram-output.cc
    ngram-shrink.cc
    util.cc
)

set(NGRAM_HEADERS
    hist-arc.h
    hist-mapper.h
    lexicographic-map.h
    ngram-absolute.h
    ngram-bayes-model-merge.h
    ngram-complete.h
    ngram-context.h
    ngram-context-merge.h
    ngram-context-prune.h
    ngram-count.h
    ngram-count-merge.h
    ngram-count-of-counts.h
    ngram-count-prune.h
    ngram-hist-merge.h
    ngram-input.h
    ngram-katz.h
    ngram-kneser-ney.h
    ngram-list-prune.h
    ngram-make.h
    ngram-marginalize.h
    ngram-merge.h
    ngram-model.h
    ngram-model-merge.h
    ngram-mutable-model.h
    ngram-output.h
    ngram-randgen.h
    ngram-relentropy.h
    ngram-replace-merge.h
    ngram-seymore-shrink.h
    ngram-shrink.h
    ngram-split.h
    ngram-transfer.h
    ngram-unsmoothed.h
    ngram-witten-bell.h
    util.h
)

grm_add_library(grm_ngram ${NGRAM_SRCS})

target_include_directories(
  grm_ngram PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
  grm_ngram
  PUBLIC openfst::fst
         openfst::far
         absl::strings
         absl::log
)

# hist-arc library and module
add_library(grm_hist-arc hist-arc.cc)
target_include_directories(grm_hist-arc
                           PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)
target_link_libraries(grm_hist-arc PUBLIC openfst::fst openfst::fstscript)

add_library(hist-arc MODULE hist-arc.cc)
target_include_directories(hist-arc
                           PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)
target_link_libraries(hist-arc PRIVATE openfst::fst openfst::fstscript)
set_target_properties(
  hist-arc
  PROPERTIES PREFIX ""
) # Produce hist-arc.so, not libhist-arc.so

opengrm_install(
  TARGETS
  grm_ngram
  grm_hist-arc
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
  TARGETS
  hist-arc
  LIBRARY
  DESTINATION
  ${CMAKE_INSTALL_LIBDIR}/opengrm
) # Or wherever plugins go

opengrm_install(
  FILES
  ${NGRAM_HEADERS}
  DESTINATION
  ${CMAKE_INSTALL_INCLUDEDIR}/opengrm/ngram
)

if(OPENGRM_ENABLE_BIN)
  set(NGRAM_TOOLS
      ngramapply
      ngramcontext
      ngramcount
      ngramhisttest
      ngraminfo
      ngrammake
      ngrammarginalize
      ngrammerge
      ngramperplexity
      ngramprint
      ngramrandgen
      ngramrandtest
      ngramread
      ngramshrink
      ngramsort
      ngramsplit
      ngramsymbols
      ngramtransfer
  )

  foreach(_tool ${NGRAM_TOOLS})
    add_executable(${_tool} ${_tool}.cc ${_tool}-main.cc)
    target_link_libraries(
      ${_tool}
      PRIVATE grm_ngram
              grm_hist-arc
              openfst::fst
              absl::flags
    )
    opengrm_install(
      TARGETS
      ${_tool}
      RUNTIME
      DESTINATION
      ${CMAKE_INSTALL_BINDIR}
    )
  endforeach()
endif()

if(OPENGRM_BUILD_TESTS)
  add_executable(ngram-count_test ngram-count_test.cc)
  target_link_libraries(ngram-count_test PRIVATE grm_ngram GTest::gmock_main)
  grm_configure_test_target(ngram-count_test)
  gtest_discover_tests(
    ngram-count_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  add_executable(ngram-bayes-model-merge_test ngram-bayes-model-merge_test.cc)
  target_link_libraries(ngram-bayes-model-merge_test PRIVATE grm_ngram
                                                             GTest::gmock_main
  )
  grm_configure_test_target(ngram-bayes-model-merge_test)
  gtest_discover_tests(
    ngram-bayes-model-merge_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  add_executable(ngram-replace-merge_test ngram-replace-merge_test.cc)
  target_link_libraries(ngram-replace-merge_test PRIVATE grm_ngram
                                                         GTest::gmock_main
  )
  grm_configure_test_target(ngram-replace-merge_test)
  gtest_discover_tests(
    ngram-replace-merge_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  add_executable(ngram-shrink_test ngram-shrink_test.cc)
  target_link_libraries(ngram-shrink_test PRIVATE grm_ngram GTest::gmock_main)
  grm_configure_test_target(ngram-shrink_test)
  gtest_discover_tests(
    ngram-shrink_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )
endif()
