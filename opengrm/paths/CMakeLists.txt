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

grm_add_library(grm_paths INTERFACE)

target_include_directories(
  grm_paths INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(grm_paths INTERFACE openfst::fst # Or whatever OpenFst
                                                       # target we use
)

# Script layer
add_library(grm_pathsscript pathsscript.cc)

target_include_directories(
  grm_pathsscript PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(grm_pathsscript PUBLIC grm_paths openfst::fstscript)

opengrm_install(
  TARGETS grm_pathsscript
  EXPORT OpenGrmTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

opengrm_install(FILES paths.h pathsscript.h
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/opengrm/paths
)

if(OPENGRM_BUILD_TESTS)
  add_executable(paths_test paths_test.cc)
  target_link_libraries(
    paths_test
    PRIVATE grm_paths
            grm_pathsscript
            GTest::gmock_main
            absl::strings
  )
  grm_configure_test_target(paths_test)
  gtest_discover_tests(
    paths_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )
endif()
