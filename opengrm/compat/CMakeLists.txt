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

set(COMPAT_SRCS file.cc)

set(COMPAT_HEADERS file.h)

grm_add_library(grm_compat ${COMPAT_SRCS})

target_link_libraries(
  grm_compat
  PUBLIC absl::status absl::statusor absl::strings
  PRIVATE absl::log
)

opengrm_install(
  TARGETS grm_compat
  EXPORT OpenGrmTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

opengrm_install(
  FILES ${COMPAT_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/opengrm/compat
)

if(OPENGRM_BUILD_TESTS)
  add_executable(file_test file_test.cc)
  target_link_libraries(file_test PRIVATE grm_compat openfst::fst
                                          GTest::gtest_main
  )
  grm_configure_test_target(file_test)
  gtest_discover_tests(
    file_test
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )
endif()
