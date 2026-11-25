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

# Canonical OpenGrm CMake helper functions. Centralizes library declaration,
# export metadata, shared-library versioning, build/install include interfaces,
# and universal namespaced target aliasing.

function(grm_add_library TARGET_NAME)
  # Delegate to built-in add_library supporting exact signature.
  add_library(${ARGV})

  # Derive modern lowercase imported target name. Internally, OpenGrm targets
  # are prefixed with 'grm_' (e.g., grm_ngram, grm_sfst), or 'thrax_' (e.g.,
  # thrax_core). External consumers expect prefix-stripped target names or
  # standard imports (e.g., opengrm::ngram, opengrm::thrax_core).
  if(TARGET_NAME MATCHES "^grm_(.+)$")
    set(EXPORT_NAME "${CMAKE_MATCH_1}")
  else()
    set(EXPORT_NAME "${TARGET_NAME}")
  endif()

  set_target_properties(
    ${TARGET_NAME}
    PROPERTIES EXPORT_NAME ${EXPORT_NAME}
  )

  get_target_property(_target_type ${TARGET_NAME} TYPE)
  if(_target_type STREQUAL "SHARED_LIBRARY")
    set_target_properties(
      ${TARGET_NAME}
      PROPERTIES SOVERSION ${OPENGRM_SOVERSION}
    )
  endif()

  # Attach root include directories (<repo>/opengrm/... in the build tree and
  # CMAKE_INSTALL_INCLUDEDIR from GNUInstallDirs on install). Header-only
  # INTERFACE libraries require INTERFACE scope; compiled targets use PUBLIC.
  set(_include_scope PUBLIC)
  if(_target_type STREQUAL "INTERFACE_LIBRARY")
    set(_include_scope INTERFACE)
  endif()
  target_include_directories(
    ${TARGET_NAME}
    ${_include_scope}
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  # Automatically inject fail-safe ALIAS bridge.
  add_library(opengrm::${EXPORT_NAME} ALIAS ${TARGET_NAME})
endfunction()

# Configures a test target with the test temp directory wrapper to prevent
# collisions in parallel runs.
function(grm_configure_test_target TARGET_NAME)
  set_target_properties(
    ${TARGET_NAME}
    PROPERTIES
      CROSSCOMPILING_EMULATOR
        "${PROJECT_SOURCE_DIR}/scripts/run_with_test_tmpdir.sh"
  )
endfunction()
