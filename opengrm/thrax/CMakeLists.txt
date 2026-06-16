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

add_subdirectory(ast)

set(THRAX_HEADERS
    abstract-grm-manager.h
    compiler.h
    grm-compiler.h
    grm-manager.h
    lexer.h
    rewrite-tester.h
)

# Core lib containing abstract-grm-manager, flags, and compatibility file
# helpers
grm_add_library(thrax_core flags.cc compat/file.cc)
target_include_directories(thrax_core
                           PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)
target_link_libraries(
  thrax_core
  PUBLIC openfst::fst
         openfst::far
         openfst::pdt
         openfst::mpdt
         grm_rewrite # For parentheses.h
         absl::flags
         absl::strings
         absl::status
)

add_subdirectory(walker)

# Bison parser
find_package(BISON REQUIRED)
bison_target(thrax_parser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cc
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.h
)

# Compiler lib
add_library(thrax_compiler_lib grm-compiler.cc lexer.cc
                               ${BISON_thrax_parser_OUTPUTS}
)
target_include_directories(thrax_compiler_lib
                           PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
                                  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)
target_link_libraries(
  thrax_compiler_lib
  PUBLIC thrax_core
         thrax_ast
         thrax_walker
         openfst::fst
         absl::container
)

# Rewrite tester lib
add_library(thrax_rewrite-tester_lib rewrite-tester.cc)
target_include_directories(thrax_rewrite-tester_lib
                           PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)
target_link_libraries(thrax_rewrite-tester_lib
                      PUBLIC thrax_core thrax_walker # For walker/util/function
                             grm_paths
)

opengrm_install(
  TARGETS thrax_core thrax_compiler_lib thrax_rewrite-tester_lib
  EXPORT OpenGrmTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

opengrm_install(FILES ${THRAX_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/parser.h
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/opengrm/thrax
)

if(OPENGRM_ENABLE_BIN)
  # compiler
  add_executable(
    thraxcompiler
    compiler.cc
    compiler-log.cc
    compiler-log64.cc
    compiler-stdarc.cc
  )
  target_link_libraries(
    thraxcompiler
    PRIVATE ${FORCE_LOAD_START}
            thrax_compiler_lib
            thrax_walker
            openfst::fst
            openfst::far
            openfst::fstscript
            openfst::pdt
            openfst::mpdt
            ${FORCE_LOAD_END}
            absl::flags
  )
  opengrm_install(TARGETS thraxcompiler RUNTIME DESTINATION
                                                ${CMAKE_INSTALL_BINDIR}
  )

  # random-generator
  add_executable(thraxrandom-generator random-generator.cc)
  target_link_libraries(
    thraxrandom-generator
    PRIVATE ${FORCE_LOAD_START}
            thrax_walker
            grm_rewrite
            openfst::fst
            openfst::far
            openfst::fstscript
            ${FORCE_LOAD_END}
            thrax_core
            absl::flags
  )
  opengrm_install(TARGETS thraxrandom-generator RUNTIME DESTINATION
                                                        ${CMAKE_INSTALL_BINDIR}
  )

  # rewrite-tester
  add_executable(thraxrewrite-tester rewrite-tester-main.cc)
  target_link_libraries(
    thraxrewrite-tester
    PRIVATE ${FORCE_LOAD_START}
            thrax_rewrite-tester_lib
            thrax_walker
            openfst::fst
            openfst::far
            openfst::fstscript
            ${FORCE_LOAD_END}
            absl::flags
  )
  opengrm_install(TARGETS thraxrewrite-tester RUNTIME DESTINATION
                                                      ${CMAKE_INSTALL_BINDIR}
  )
endif()

if(OPENGRM_BUILD_TESTS)
  add_subdirectory(test)
endif()
