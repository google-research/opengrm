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

cmake_minimum_required(VERSION 3.22)

# Custom "Dev" build type
set(CMAKE_CXX_FLAGS_DEV_INIT "-O1")

project(
  opengrm
  VERSION 1.4.1
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)

# Shared library versioning (arbitrary for now, match version maybe)
set(OPENGRM_SOVERSION 14)

# Options
option(OPENGRM_BUILD_TESTS "Enable OpenGrm tests" ON)
option(OPENGRM_RUN_SLOW_TESTS "Run slow integration tests" OFF)
option(OPENGRM_ENABLE_BIN "Enable executables" ON)
option(OPENGRM_ENABLE_INSTALL "Enable installation of OpenGrm" OFF)

# Build static libraries by default. Registration issues in static builds are
# handled via linker hacks (OPENGRM_ALWAYS_LINK_START/OPENGRM_ALWAYS_LINK_END).
# Set to ON to build shared libraries.
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Dependency options
option(OPENGRM_USE_SYSTEM_ABSEIL
       "Use installed Abseil-cpp instead of downloading" OFF
)
option(OPENGRM_USE_SYSTEM_OPENFST
       "Use installed OpenFst instead of downloading" OFF
)
option(OPENGRM_USE_SYSTEM_GTEST
       "Use installed GoogleTest instead of downloading" OFF
)

# Component Enable/Disable Options
option(OPENGRM_ENABLE_BAUMWELCH "Enable Baum-Welch extensions" OFF)
option(OPENGRM_ENABLE_NGRAM "Enable NGram extensions" OFF)
option(OPENGRM_ENABLE_SFST "Enable SFST extensions" OFF)
option(OPENGRM_ENABLE_THRAX "Enable Thrax compilation and tools" OFF)
option(OPENGRM_ENABLE_PYNINI "Enable Pynini extension" OFF)

# Compiler Warnings
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(
    -Wall
    -Wno-deprecated-declarations
    -Wno-overloaded-virtual
    -Wno-sign-compare
    # Temporarily disabled warnings that we would like to turn on.
    -Wno-defaulted-function-deleted
    -Wno-nullability-completeness
  )
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-Wno-changes-meaning" HAS_WNO_CHANGES_MEANING)
  if(HAS_WNO_CHANGES_MEANING)
    add_compile_options(-Wno-changes-meaning)
  endif()
endif()

# Linker flags to force loading of static/shared libraries that use
# self-registering objects. Without these, the linker will optimize out/discard
# the libraries because no symbols from them are explicitly referenced at
# compile time. TODO: Around 2027-05-01, when the CMake minimum version can be
# bumped to 3.24 (per
# https://github.com/google/oss-policies-info/blob/main/foundational-cxx-support-matrix.md),
# remove these variables and replace usage in target_link_libraries with
# $<LINK_LIBRARY:WHOLE_ARCHIVE,lib>.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  if(BUILD_SHARED_LIBS)
    set(OPENGRM_ALWAYS_LINK_START "-Wl,--no-as-needed")
    set(OPENGRM_ALWAYS_LINK_END "-Wl,--as-needed")
  else()
    set(OPENGRM_ALWAYS_LINK_START "-Wl,--whole-archive")
    set(OPENGRM_ALWAYS_LINK_END "-Wl,--no-whole-archive")
  endif()
else()
  set(OPENGRM_ALWAYS_LINK_START "")
  set(OPENGRM_ALWAYS_LINK_END "")
endif()

include(FetchContent)
include(GNUInstallDirs)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenGrmHelpers.cmake)

# Helper macro for conditional installation
function(opengrm_install)
  if(OPENGRM_ENABLE_INSTALL)
    install(${ARGV})
  endif()
endfunction()

# Abseil
if(OPENGRM_USE_SYSTEM_ABSEIL)
  find_package(absl REQUIRED)
else()
  set(ABSL_ENABLE_INSTALL OFF)
  set(ABSL_PROPAGATE_CXX_STD ON)
  set(ABSL_BUILD_TESTING OFF)
  FetchContent_Declare(
    abseil-cpp
    URL https://github.com/abseil/abseil-cpp/archive/refs/tags/20260526.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(abseil-cpp)
endif()
# Bridge pseudo-target absl::container to actual Abseil container libraries
add_library(absl::container INTERFACE IMPORTED)
set_target_properties(
  absl::container
  PROPERTIES
    INTERFACE_LINK_LIBRARIES
    "absl::flat_hash_map;absl::flat_hash_set;absl::node_hash_map;absl::node_hash_set;absl::btree;absl::inlined_vector"
)

# OpenFst
if(OPENGRM_USE_SYSTEM_OPENFST)
  find_package(openfst REQUIRED)
else()
  # Bundle OpenFst
  set(OPENFST_BUILD_TESTS OFF CACHE BOOL "Build OpenFst tests")
  set(OPENFST_ENABLE_BIN OFF CACHE BOOL "Enable fst::script and executables")
  set(OPENFST_ENABLE_INSTALL OFF CACHE BOOL "Enable installation of OpenFst")
  set(OPENFST_ENABLE_SPECIAL OFF CACHE BOOL "Enable special-matcher extensions")
  # We might need to enable FAR or other specific OpenFst extensions if needed
  # by OpenGrm OpenGrm highly depends on FAR, MPDT, PDT
  set(OPENFST_ENABLE_GRM ON CACHE BOOL "Enable all dependencies of OpenGrm")
  set(OPENFST_ENABLE_PDT ON CACHE BOOL "Enable PDT extensions")
  set(OPENFST_ENABLE_MPDT ON CACHE BOOL "Enable MPDT extensions")
  set(OPENFST_ENABLE_FAR ON CACHE BOOL "Enable FAR extensions")
  if(OPENGRM_ENABLE_PYNINI)
    set(OPENFST_ENABLE_PYTHON ON CACHE BOOL "Enable Python extension" FORCE)
  endif()
  if(OPENGRM_ENABLE_BAUMWELCH)
    set(OPENFST_ENABLE_SPECIAL ON
        CACHE BOOL "Enable special-matcher extensions" FORCE
    )
    set(OPENFST_ENABLE_BIN ON CACHE BOOL "Enable fst::script and executables"
                                    FORCE
    )
    set(OPENFST_ENABLE_COMPACT_FSTS ON
        CACHE BOOL "Enable CompactFst extensions" FORCE
    )
  endif()

  # We use OpenFst from the current HEAD until the first release is cut.
  FetchContent_Declare(
    openfst
    GIT_REPOSITORY https://github.com/google-research/openfst.git
    GIT_TAG 8e3e92b38afb8d0b27015bdccbd6f6ef479d9400
  )
  FetchContent_MakeAvailable(openfst)
endif()

# GoogleTest
if(OPENGRM_BUILD_TESTS)
  enable_testing()
  if(OPENGRM_USE_SYSTEM_GTEST)
    find_package(GTest REQUIRED)
  else()
    set(INSTALL_GTEST OFF)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)
  endif()
  include(GoogleTest)
endif()

# Bison (needed by Thrax)
if(OPENGRM_ENABLE_THRAX)
  # Require Bison 3.0+ to support modern Bison directives (e.g. %define
  # api.pure) used in parser.y, which fail on older versions like macOS default
  # Bison 2.3.
  find_package(BISON 3.0 REQUIRED)
endif()

# Core OpenGrm
add_subdirectory(opengrm)

# Installation of the export set
opengrm_install(
  EXPORT
  OpenGrmTargets
  FILE
  OpenGrmTargets.cmake
  NAMESPACE
  opengrm::
  DESTINATION
  ${CMAKE_INSTALL_LIBDIR}/cmake/opengrm
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  OpenGrmConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenGrmConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/OpenGrmConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/opengrm
)

opengrm_install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/OpenGrmConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/OpenGrmConfigVersion.cmake
  DESTINATION
  ${CMAKE_INSTALL_LIBDIR}/cmake/opengrm
)
