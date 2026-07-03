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

get_filename_component(OPENGRM_BUILD_ROOT "${CMAKE_CURRENT_BINARY_DIR}/../.."
                       ABSOLUTE
)

if(TARGET protobuf::protoc)
  set(OPENGRM_PROTOC_EXE protobuf::protoc)
else()
  find_program(OPENGRM_PROTOC_EXE protoc REQUIRED)
endif()

set(PROTOBUF_IMPORT_DIRS "")
if(DEFINED protobuf_SOURCE_DIR)
  list(
    APPEND
    PROTOBUF_IMPORT_DIRS
    "-I"
    "${protobuf_SOURCE_DIR}/src"
  )
elseif(DEFINED Protobuf_INCLUDE_DIRS)
  foreach(_dir ${Protobuf_INCLUDE_DIRS})
    list(
      APPEND
      PROTOBUF_IMPORT_DIRS
      "-I"
      "${_dir}"
    )
  endforeach()
endif()

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/testdata.pb.h"
         "${CMAKE_CURRENT_BINARY_DIR}/testdata.pb.cc"
  COMMAND
    ${OPENGRM_PROTOC_EXE} ARGS --cpp_out "${CMAKE_CURRENT_BINARY_DIR}" -I
    "${CMAKE_CURRENT_SOURCE_DIR}" ${PROTOBUF_IMPORT_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/testdata.proto"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/testdata.proto" ${OPENGRM_PROTOC_EXE}
  COMMENT "Running C++ protocol buffer compiler on testdata.proto"
  VERBATIM
)

grm_add_library(
  grm_testdata_cc_proto
  testdata.proto
  "${CMAKE_CURRENT_BINARY_DIR}/testdata.pb.cc"
  "${CMAKE_CURRENT_BINARY_DIR}/testdata.pb.h"
)
set_target_properties(
  grm_testdata_cc_proto
  PROPERTIES LINKER_LANGUAGE CXX
)

target_include_directories(
  grm_testdata_cc_proto
  PUBLIC $<BUILD_INTERFACE:${OPENGRM_BUILD_ROOT}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
         $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
         $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
         $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(grm_testdata_cc_proto PUBLIC protobuf::libprotobuf)

grm_add_library(grm_test_textproto_lib test_textproto.cc)
set_target_properties(
  grm_test_textproto_lib
  PROPERTIES LINKER_LANGUAGE CXX
)

target_include_directories(
  grm_test_textproto_lib
  PUBLIC $<BUILD_INTERFACE:${OPENGRM_BUILD_ROOT}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
         $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
         $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
         $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(
  grm_test_textproto_lib
  PUBLIC grm_testdata_cc_proto
         grm_compat
         grm_rewrite
         openfst::fst
         openfst::far
         openfst::fstscript
         absl::algorithm_container
         absl::check
         absl::flags
         absl::flags_parse
         absl::log
         absl::status
         absl::statusor
         absl::strings
         protobuf::libprotobuf
         GTest::gmock
         GTest::gtest
)

add_executable(test_textproto $<TARGET_OBJECTS:grm_test_textproto_lib>)
set_target_properties(
  test_textproto
  PROPERTIES LINKER_LANGUAGE CXX
)

target_link_libraries(
  test_textproto
  PRIVATE grm_test_textproto_lib
          ${OPENGRM_ALWAYS_LINK_START}
          grm_compat
          grm_rewrite
          openfst::fst
          openfst::far
          openfst::fstscript
          ${OPENGRM_ALWAYS_LINK_END}
          grm_testdata_cc_proto
          absl::algorithm_container
          absl::check
          absl::flags
          absl::flags_parse
          absl::log
          absl::status
          absl::statusor
          absl::strings
          protobuf::libprotobuf
          GTest::gmock
          GTest::gtest
)

grm_configure_test_target(test_textproto)
