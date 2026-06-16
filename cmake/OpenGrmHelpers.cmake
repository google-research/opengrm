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

# Canonical OpenGrm CMake helper macros. Centralizes library declaration and
# universal namespaced target aliasing.

function(grm_add_library TARGET_NAME)
  # Delegate to built-in add_library supporting exact signature.
  add_library(${ARGV})

  # Derive modern lowercase imported target name. Internally, OpenGrm targets
  # are prefixed with 'grm_' (e.g., grm_ngram, grm_sfst), or 'thrax_' (e.g.,
  # thrax_core). External consumers expect prefix-stripped target names or
  # standard imports (e.g., opengrm::ngram, opengrm::thrax_core).
  set(EXPORT_NAME "")
  if(TARGET_NAME MATCHES "^grm_(.+)$")
    set(EXPORT_NAME "${CMAKE_MATCH_1}")
  elseif(TARGET_NAME STREQUAL "thrax_core")
    set(EXPORT_NAME "thrax_core")
  endif()

  # Automatically inject fail-safe ALIAS bridge.
  if(NOT
     EXPORT_NAME
     STREQUAL
     ""
  )
    add_library(opengrm::${EXPORT_NAME} ALIAS ${TARGET_NAME})
  endif()
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
