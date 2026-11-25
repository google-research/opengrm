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

# Shared library versioning
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
option(OPENGRM_USE_SYSTEM_PROTOBUF
       "Use installed Protobuf instead of downloading" OFF
)

# Component Enable/Disable Options
option(OPENGRM_ENABLE_BAUMWELCH "Enable Baum-Welch extensions" OFF)
option(OPENGRM_ENABLE_NGRAM "Enable NGram extensions" OFF)
option(OPENGRM_ENABLE_SFST "Enable SFST extensions" OFF)
option(OPENGRM_ENABLE_THRAX "Enable Thrax compilation and tools" OFF)
option(OPENGRM_ENABLE_PYNINI "Enable Pynini extension" OFF)

# Installation requires system Abseil and OpenFst to avoid vendoring/export
# conflicts with FetchContent subprojects.
if(OPENGRM_ENABLE_INSTALL AND (NOT OPENGRM_USE_SYSTEM_ABSEIL
                               OR NOT OPENGRM_USE_SYSTEM_OPENFST)
)
  message(
    FATAL_ERROR
      "OPENGRM_ENABLE_INSTALL requires OPENGRM_USE_SYSTEM_ABSEIL=ON and "
      "OPENGRM_USE_SYSTEM_OPENFST=ON. OpenGrm cannot install its own Abseil or "
      "OpenFst copies. Please install them separately or disable installation "
      "(-DOPENGRM_ENABLE_INSTALL=OFF)."
  )
endif()

# Pynini and OpenFst's pywrapfst are separate Python extension modules that
# share OpenFst and Abseil state; static linking duplicates Abseil flags and
# FST/arc registries across both DSOs and crashes on import.
if(OPENGRM_ENABLE_PYNINI AND NOT BUILD_SHARED_LIBS)
  message(
    FATAL_ERROR
      "OPENGRM_ENABLE_PYNINI requires BUILD_SHARED_LIBS=ON. "
      "Static builds duplicate OpenFst and Abseil registries across the "
      "pywrapfst and pynini Python extensions. Please enable shared libraries "
      "(-DBUILD_SHARED_LIBS=ON) or disable Pynini "
      "(-DOPENGRM_ENABLE_PYNINI=OFF)."
  )
endif()

# Building Pynini requires OpenFst's Cython .pxd headers, which are only
# available from the bundled FetchContent OpenFst source tree.
if(OPENGRM_ENABLE_PYNINI AND OPENGRM_USE_SYSTEM_OPENFST)
  message(
    FATAL_ERROR
      "OPENGRM_ENABLE_PYNINI requires bundled OpenFst "
      "(-DOPENGRM_USE_SYSTEM_OPENFST=OFF) because Cython needs OpenFst's "
      ".pxd headers. To install Pynini, use `pip install .`, or disable "
      "Pynini (-DOPENGRM_ENABLE_PYNINI=OFF)."
  )
endif()

# Compiler Warnings
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(
    -Wall
    -Wno-deprecated-declarations
    -Wno-sign-compare
    "$<$<COMPILE_LANGUAGE:CXX>:-Wno-overloaded-virtual>"
  )
  include(CheckCXXCompilerFlag)
  # Probe the positive -W flags because GCC silently accepts unknown -Wno-*
  # flags during isolated feature checks and later emits notes on warnings.
  #
  # Temporarily disabled warnings that we would like to turn on.
  check_cxx_compiler_flag("-Wdefaulted-function-deleted"
                          HAS_WDEFAULTED_FUNCTION_DELETED
  )
  if(HAS_WDEFAULTED_FUNCTION_DELETED)
    add_compile_options(
      "$<$<COMPILE_LANGUAGE:CXX>:-Wno-defaulted-function-deleted>"
    )
  endif()
  check_cxx_compiler_flag("-Wchanges-meaning" HAS_WCHANGES_MEANING)
  if(HAS_WCHANGES_MEANING)
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wno-changes-meaning>")
  endif()
  check_cxx_compiler_flag("-Wnullability-completeness"
                          HAS_WNULLABILITY_COMPLETENESS
  )
  if(HAS_WNULLABILITY_COMPLETENESS)
    add_compile_options(
      "$<$<COMPILE_LANGUAGE:CXX>:-Wno-nullability-completeness>"
    )
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
    URL https://github.com/abseil/abseil-cpp/releases/download/20260817.0/abseil-cpp-20260817.0.tar.gz
    URL_HASH
      SHA256=f7e05179df39c45434cad433f5783840bb3788ef322976f9138bc6b72b3a107d
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(abseil-cpp)
endif()

# Protobuf (needed by testing/test_textproto)
if(OPENGRM_BUILD_TESTS AND OPENGRM_ENABLE_THRAX)
  if(OPENGRM_USE_SYSTEM_PROTOBUF)
    find_package(Protobuf REQUIRED CONFIG)
    if(NOT TARGET protobuf::libprotobuf AND TARGET Protobuf::libprotobuf)
      add_library(protobuf::libprotobuf ALIAS Protobuf::libprotobuf)
    endif()
  else()
    set(protobuf_BUILD_TESTS OFF CACHE BOOL "Build protobuf tests" FORCE)
    set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "Build protoc" FORCE)
    set(protobuf_INSTALL OFF CACHE BOOL "Install protobuf" FORCE)
    set(protobuf_ABSL_PROVIDER "package" CACHE STRING "Use external Abseil"
                                               FORCE
    )
    if(NOT OPENGRM_USE_SYSTEM_ABSEIL)
      # Ensure Protobuf finds Abseil targets created by FetchContent
      set(absl_FOUND TRUE)
    endif()
    FetchContent_Declare(
      protobuf
      URL https://github.com/protocolbuffers/protobuf/releases/download/v35.1/protobuf-35.1.tar.gz
      URL_HASH
        SHA256=f0b6838e7522a8da96126d487068c959bc624926368f3024ac8fd03abd0a1ac4
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(protobuf)
  endif()
endif()

# OpenFst
if(OPENGRM_USE_SYSTEM_OPENFST)
  find_package(OpenFst REQUIRED)
else()
  # Bundle OpenFst. OPENFST_ENABLE_GRM=ON enables FAR, PDT, and MPDT.
  set(OPENFST_BUILD_TESTS OFF CACHE BOOL "Build OpenFst tests" FORCE)
  set(OPENFST_ENABLE_INSTALL OFF CACHE BOOL "Enable installation of OpenFst"
                                       FORCE
  )
  set(OPENFST_ENABLE_GRM ON CACHE BOOL "Enable all dependencies of OpenGrm"
                                  FORCE
  )
  set(OPENFST_ENABLE_PYTHON ${OPENGRM_ENABLE_PYNINI}
      CACHE BOOL "Enable Python extension" FORCE
  )
  # Baum-Welch tests require openfst::phi-fst (OPENFST_ENABLE_SPECIAL) and
  # compact string FSTs (OPENFST_ENABLE_COMPACT_FSTS). OpenFst's
  # extensions/special unconditionally builds fstspecial against
  # fstconvert-main, which is only defined when OPENFST_ENABLE_BIN is ON.
  set(_opengrm_need_baumwelch_openfst_ext OFF)
  if(OPENGRM_ENABLE_BAUMWELCH AND OPENGRM_BUILD_TESTS)
    set(_opengrm_need_baumwelch_openfst_ext ON)
  endif()
  set(OPENFST_ENABLE_SPECIAL ${_opengrm_need_baumwelch_openfst_ext}
      CACHE BOOL "Enable special-matcher extensions" FORCE
  )
  set(OPENFST_ENABLE_BIN ${_opengrm_need_baumwelch_openfst_ext}
      CACHE BOOL "Enable fst::script and executables" FORCE
  )
  set(OPENFST_ENABLE_COMPACT_FSTS ${_opengrm_need_baumwelch_openfst_ext}
      CACHE BOOL "Enable CompactFst extensions" FORCE
  )

  # We use OpenFst from the current HEAD until the first release is cut.
  FetchContent_Declare(
    openfst
    GIT_REPOSITORY https://github.com/google-research/openfst.git
    GIT_TAG e32e48f76ed07c80e7cfb923ad8e4811d6c14128 # 11/23/26.
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
      URL https://github.com/google/googletest/releases/download/v1.18.0/googletest-1.18.0.tar.gz
      URL_HASH
        SHA256=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5
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

# Installation of the export set and package configuration
if(OPENGRM_ENABLE_INSTALL)
  opengrm_install(
    EXPORT OpenGrmTargets
    FILE OpenGrmTargets.cmake
    NAMESPACE opengrm::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/opengrm
  )

  opengrm_install(
    FILES LICENSE
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
  )

  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/opengrm-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
  )

  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenGrmConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/opengrm-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/opengrm
  )

  opengrm_install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/opengrm-config.cmake
          ${CMAKE_CURRENT_BINARY_DIR}/opengrm-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/opengrm
  )
endif()
