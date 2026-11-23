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

add_library(grm_testdata_cc_proto STATIC
            "${CMAKE_CURRENT_BINARY_DIR}/testdata.pb.cc"
            "${CMAKE_CURRENT_BINARY_DIR}/testdata.pb.h"
)
target_include_directories(grm_testdata_cc_proto PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(grm_testdata_cc_proto PUBLIC protobuf::libprotobuf)

add_executable(test_textproto test_textproto.cc)
target_include_directories(test_textproto PRIVATE "${PROJECT_SOURCE_DIR}")
target_link_libraries(
  test_textproto
  PRIVATE ${OPENGRM_ALWAYS_LINK_START}
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

if(OPENGRM_ENABLE_BIN)
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/test_grammar.far"
    COMMAND
      thraxcompiler
      "--input_grammar=${CMAKE_CURRENT_SOURCE_DIR}/test_grammar.grm"
      "--output_far=${CMAKE_CURRENT_BINARY_DIR}/test_grammar.far"
    DEPENDS thraxcompiler "${CMAKE_CURRENT_SOURCE_DIR}/test_grammar.grm"
    COMMENT "Compiling test_grammar.grm"
    VERBATIM
  )
  add_custom_target(test_grammar_far ALL
                    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/test_grammar.far"
  )
  add_test(
    NAME build_defs_test
    COMMAND
      test_textproto "--far_path=${CMAKE_CURRENT_BINARY_DIR}/test_grammar.far"
      "--textproto_path=${CMAKE_CURRENT_SOURCE_DIR}/testdata/build_defs.textproto"
  )
endif()
