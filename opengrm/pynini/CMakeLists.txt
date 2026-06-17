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

find_package(
  Python3
  COMPONENTS Interpreter Development
  REQUIRED
)
find_program(CYTHON_EXECUTABLE NAMES cython3 cython REQUIRED)

if(Python3_FOUND AND CYTHON_EXECUTABLE)
  message(STATUS "Found Python3: ${Python3_EXECUTABLE}")
  message(STATUS "Found Cython: ${CYTHON_EXECUTABLE}")

  set(PYNINI_CC ${CMAKE_CURRENT_BINARY_DIR}/pynini.cc)

  # Set up Cython include paths
  set(CYTHON_INCLUDES -I ${CMAKE_SOURCE_DIR})
  if(openfst_SOURCE_DIR)
    list(
      APPEND
      CYTHON_INCLUDES
      -I
      ${openfst_SOURCE_DIR}
    )
  endif()

  # Cython command
  add_custom_command(
    OUTPUT ${PYNINI_CC}
    COMMAND ${CYTHON_EXECUTABLE} -3 --cplus ${CYTHON_INCLUDES} -o ${PYNINI_CC}
            ${CMAKE_CURRENT_SOURCE_DIR}/pynini.pyx
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pynini.pyx
            ${CMAKE_CURRENT_SOURCE_DIR}/cpynini.pxd
    COMMENT "Generating C++ source from Cython file for Pynini"
  )

  # Python module
  add_library(pynini MODULE ${PYNINI_CC})
  set_target_properties(
    pynini
    PROPERTIES PREFIX ""
  )

  target_include_directories(pynini PRIVATE ${CMAKE_SOURCE_DIR}
                                            ${Python3_INCLUDE_DIRS}
  )

  target_link_libraries(
    pynini
    PRIVATE grm_operatorsscript
            grm_pathsscript
            grm_stringscript
            openfst::mpdtscript
            openfst::pdtscript
            openfst::fst
            openfst::fstscript
            Python3::Module
  )

  if(UNIX AND NOT APPLE)
    target_link_options(pynini PRIVATE "-Wl,--no-as-needed")
  endif()

  # Package structure for testing
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/opengrm)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/opengrm/pynini)

  # Copy __init__.py from parent directory
  execute_process(
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/../__init__.py
      ${CMAKE_CURRENT_BINARY_DIR}/opengrm/__init__.py
  )

  # Create pynini/__init__.py
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/opengrm/pynini/__init__.py
       "from .pynini import *\n"
  )

  # Copy runfiles.py
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/open_source/runfiles.py)
    set(RUNFILES_PY_SRC ${CMAKE_CURRENT_SOURCE_DIR}/open_source/runfiles.py)
  else()
    set(RUNFILES_PY_SRC ${CMAKE_CURRENT_SOURCE_DIR}/runfiles.py)
  endif()
  configure_file(${RUNFILES_PY_SRC}
                 ${CMAKE_CURRENT_BINARY_DIR}/opengrm/pynini/runfiles.py
                 COPYONLY
  )

  add_custom_command(
    TARGET pynini
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:pynini>
            ${CMAKE_CURRENT_BINARY_DIR}/opengrm/pynini/
    COMMENT "Copying pynini to opengrm/pynini/ package directory"
  )

  # Copy lib and export directories
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/lib
            ${CMAKE_CURRENT_BINARY_DIR}/opengrm/pynini/lib
  )
  execute_process(
    COMMAND
      ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/export
      ${CMAKE_CURRENT_BINARY_DIR}/opengrm/pynini/export
  )

  # Test data setup
  set(TEST_RUNFILES_DIR ${CMAKE_CURRENT_BINARY_DIR}/testrunfiles)

  function(prepare_testdata src_dir dest_rel_path)
    if(EXISTS ${src_dir})
      file(COPY ${src_dir} DESTINATION ${TEST_RUNFILES_DIR}/${dest_rel_path})
    else()
      message(WARNING "Testdata source not found: ${src_dir}")
    endif()
  endfunction()

  if(openfst_SOURCE_DIR)
    prepare_testdata(${openfst_SOURCE_DIR}/openfst/test/testdata openfst/test)
    prepare_testdata(${openfst_SOURCE_DIR}/openfst/extensions/far/testdata
                     openfst/extensions/far
    )
    prepare_testdata(${openfst_SOURCE_DIR}/openfst/extensions/mpdt/testdata
                     openfst/extensions/mpdt
    )
    prepare_testdata(${openfst_SOURCE_DIR}/openfst/extensions/pdt/testdata
                     openfst/extensions/pdt
    )
  endif()

  prepare_testdata(${CMAKE_SOURCE_DIR}/opengrm/string/testdata opengrm/string)
  prepare_testdata(${CMAKE_SOURCE_DIR}/opengrm/operators/testdata
                   opengrm/operators
  )

  if(UNIX)
    set(PATH_SEP ":")
  else()
    set(PATH_SEP ";")
  endif()

  # Set up openfst runtime path for testing. Cython generates the extensions
  # under `openfst/extensions/python`, which is the only way we could get the
  # Bazel build to work.
  set(OPENFST_TEST_ENV ${CMAKE_CURRENT_BINARY_DIR}/openfst_test_env)
  file(MAKE_DIRECTORY ${OPENFST_TEST_ENV}/openfst)
  file(MAKE_DIRECTORY ${OPENFST_TEST_ENV}/openfst/extensions)
  file(MAKE_DIRECTORY ${OPENFST_TEST_ENV}/openfst/extensions/python)

  if(openfst_SOURCE_DIR)
    configure_file(${openfst_SOURCE_DIR}/openfst/__init__.py
                   ${OPENFST_TEST_ENV}/openfst/__init__.py COPYONLY
    )
    configure_file(${openfst_SOURCE_DIR}/openfst/extensions/__init__.py
                   ${OPENFST_TEST_ENV}/openfst/extensions/__init__.py COPYONLY
    )
    configure_file(${openfst_SOURCE_DIR}/openfst/extensions/python/__init__.py
                   ${OPENFST_TEST_ENV}/openfst/extensions/python/__init__.py
                   COPYONLY
    )
    configure_file(${openfst_SOURCE_DIR}/openfst/pywrapfst.py
                   ${OPENFST_TEST_ENV}/openfst/pywrapfst.py COPYONLY
    )
  endif()

  if(TARGET pywrapfst)
    add_dependencies(pynini pywrapfst)
    add_custom_command(
      TARGET pynini
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:pywrapfst>
              ${OPENFST_TEST_ENV}/openfst/extensions/python/
      COMMENT "Copying pywrapfst to test environment structure"
    )
  endif()

  set(TEST_PYTHONPATH
      "${CMAKE_CURRENT_BINARY_DIR}${PATH_SEP}${OPENFST_TEST_ENV}"
  )

  # Tests
  add_test(NAME pynini_test COMMAND ${Python3_EXECUTABLE}
                                    ${CMAKE_CURRENT_SOURCE_DIR}/pynini_test.py
  )
  set_tests_properties(
    pynini_test
    PROPERTIES
      ENVIRONMENT
        "PYTHONPATH=${TEST_PYTHONPATH};TEST_SRCDIR=${TEST_RUNFILES_DIR}"
  )

  add_test(NAME runfiles_test
           COMMAND ${Python3_EXECUTABLE}
                   ${CMAKE_CURRENT_SOURCE_DIR}/runfiles_test.py
  )
  set_tests_properties(
    runfiles_test
    PROPERTIES
      ENVIRONMENT
        "PYTHONPATH=${TEST_PYTHONPATH};TEST_SRCDIR=${TEST_RUNFILES_DIR}"
  )

else()
  message(STATUS "Python3 or Cython not found. Skipping Pynini build.")
endif()
