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

  # Sets up Cython include paths.
  set(CYTHON_INCLUDES -I ${CMAKE_SOURCE_DIR})
  if(openfst_SOURCE_DIR)
    list(
      APPEND
      CYTHON_INCLUDES
      -I
      ${openfst_SOURCE_DIR}
    )
  endif()

  # Generates C++ source from Cython file for Pynini.
  add_custom_command(
    OUTPUT ${PYNINI_CC}
    COMMAND ${CYTHON_EXECUTABLE} -3 --cplus ${CYTHON_INCLUDES} -o ${PYNINI_CC}
            ${CMAKE_CURRENT_SOURCE_DIR}/pynini.pyx
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pynini.pyx
            ${CMAKE_CURRENT_SOURCE_DIR}/cpynini.pxd
    COMMENT "Generating C++ source from Cython file for Pynini"
  )

  # Defines the `pynini` Python extension module.
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

  # Prepares package structure for testing and installation.
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/opengrm)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/opengrm/pynini)

  # Copies `__init__.py` from parent directory.
  execute_process(
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/../__init__.py
      ${CMAKE_CURRENT_BINARY_DIR}/opengrm/__init__.py
  )

  # Creates `pynini/__init__.py`.
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/opengrm/pynini/__init__.py
       "from .pynini import *\nfrom .pynini import __version__\n"
  )

  # Copies `runfiles.py`.
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

  # Copies `lib`, `export`, and `examples` directories.
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/lib
            ${CMAKE_CURRENT_BINARY_DIR}/opengrm/pynini/lib
  )
  execute_process(
    COMMAND
      ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/export
      ${CMAKE_CURRENT_BINARY_DIR}/opengrm/pynini/export
  )
  execute_process(
    COMMAND
      ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/examples
      ${CMAKE_CURRENT_BINARY_DIR}/opengrm/pynini/examples
  )

  # Sets up `openfst` runtime path for testing and installation. Cython
  # generates the extensions under `openfst/extensions/python`, which is the
  # only way we could get the Bazel build to work.
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

  # ---------------------------------------------------------------------------
  # Installation rules for Python wheel builds (`SKBUILD`) and standalone CMake
  # installation (`OPENGRM_ENABLE_INSTALL`).
  # ---------------------------------------------------------------------------

  # Recursively collects the build-tree output directories
  # (`$<TARGET_FILE_DIR>`) of all project-built shared libraries (`grm_*`,
  # `openfst`, `abseil-cpp`).
  #
  # By default, `cmake --install` strips the build-tree RPATH from installed
  # targets. During a PEP 517 wheel build (`SKBUILD`), preserving these
  # build-tree directories in `INSTALL_RPATH` allows wheel-repair tools
  # (`auditwheel repair` on Linux and `delocate-wheel` on macOS) as well as
  # local from-source installs (`pip install .`) to locate the compiled C++
  # shared libraries in the build directory.
  function(_pynini_collect_shared_lib_dirs dir out_dirs)
    get_property(
      _targets
      DIRECTORY "${dir}"
      PROPERTY BUILDSYSTEM_TARGETS
    )
    set(_collected_dirs "${${out_dirs}}")
    foreach(_tgt IN LISTS _targets)
      if(TARGET "${_tgt}")
        get_target_property(_imported "${_tgt}" IMPORTED)
        get_target_property(_alias "${_tgt}" ALIASED_TARGET)
        get_target_property(_type "${_tgt}" TYPE)
        if(NOT _imported
           AND NOT _alias
           AND _type STREQUAL "SHARED_LIBRARY"
        )
          list(APPEND _collected_dirs "$<TARGET_FILE_DIR:${_tgt}>")
        endif()
      endif()
    endforeach()
    get_property(
      _subdirs
      DIRECTORY "${dir}"
      PROPERTY SUBDIRECTORIES
    )
    foreach(_subdir IN LISTS _subdirs)
      _pynini_collect_shared_lib_dirs("${_subdir}" _collected_dirs)
    endforeach()
    list(REMOVE_DUPLICATES _collected_dirs)
    set(${out_dirs} "${_collected_dirs}" PARENT_SCOPE)
  endfunction()

  # Installation supports two modes:
  #
  # Mode 1: PEP 517 Python wheel builds (`SKBUILD` is defined automatically by
  # the `scikit-build-core` build backend declared in `pyproject.toml`). Here
  # `CMAKE_INSTALL_PREFIX` points directly to the root of the wheel staging
  # directory, so Python packages are installed to `.`, and `INSTALL_RPATH`
  # retains the build-tree shared library directories so `auditwheel` /
  # `delocate` can resolve and bundle `libgrm_*`, `libfst*`, and `libabsl_*`
  # into the repaired wheel.
  #
  # Mode 2: Standalone CMake installation (`OPENGRM_ENABLE_INSTALL=ON` without
  # `SKBUILD`, e.g., `cmake -DOPENGRM_ENABLE_INSTALL=ON && cmake --install`).
  # Here `CMAKE_INSTALL_PREFIX` is a standard system prefix (such as
  # `/usr/local`), so Python packages are installed under
  # `lib/python<MAJOR>.<MINOR>/site-packages` (matching OpenFst's `pywrapfst`
  # CMake install layout) and `INSTALL_RPATH` points to
  # `${CMAKE_INSTALL_FULL_LIBDIR}` where the installed C++ shared libraries
  # reside.
  if(SKBUILD OR OPENGRM_ENABLE_INSTALL)
    if(SKBUILD)
      set(PYNINI_PYTHON_INSTALL_DIR ".")
      _pynini_collect_shared_lib_dirs("${CMAKE_SOURCE_DIR}"
                                      PYNINI_INSTALL_RPATH
      )
    else()
      set(PYNINI_PYTHON_INSTALL_DIR
          "lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages"
      )
      set(PYNINI_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}")
    endif()

    set_target_properties(
      pynini
      PROPERTIES INSTALL_RPATH
                 "${PYNINI_INSTALL_RPATH}"
                 INSTALL_RPATH_USE_LINK_PATH
                 TRUE
    )
    if(TARGET pywrapfst)
      set_target_properties(
        pywrapfst
        PROPERTIES INSTALL_RPATH
                   "${PYNINI_INSTALL_RPATH}"
                   INSTALL_RPATH_USE_LINK_PATH
                   TRUE
      )
    endif()

    # Creates a temporary staging directory for generated `__init__.py` wrappers
    # and PEP 561 `py.typed` marker files.
    set(WHEEL_STAGE_DIR ${CMAKE_CURRENT_BINARY_DIR}/wheel_stage)
    file(MAKE_DIRECTORY ${WHEEL_STAGE_DIR})
    file(WRITE ${WHEEL_STAGE_DIR}/empty_init.py "")
    file(WRITE ${WHEEL_STAGE_DIR}/py.typed "")

    # Section 1: Installs the canonical `opengrm.pynini` package hierarchy
    # (`from opengrm.pynini import pynini`, `from opengrm.pynini.lib import
    # ...`).
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/opengrm/__init__.py
            DESTINATION ${PYNINI_PYTHON_INSTALL_DIR}/opengrm
    )
    install(TARGETS pynini
            DESTINATION ${PYNINI_PYTHON_INSTALL_DIR}/opengrm/pynini
    )
    install(
      FILES ${CMAKE_CURRENT_BINARY_DIR}/opengrm/pynini/__init__.py
            ${CMAKE_CURRENT_BINARY_DIR}/opengrm/pynini/runfiles.py
            ${CMAKE_CURRENT_SOURCE_DIR}/pynini.pyi
            ${WHEEL_STAGE_DIR}/py.typed
      DESTINATION ${PYNINI_PYTHON_INSTALL_DIR}/opengrm/pynini
    )

    # Installs `lib`, `export`, and `examples` subpackages (excluding unit
    # tests) under `opengrm/pynini/<subpkg>`, and generates thin forwarding
    # modules under top-level `pynini/<subpkg>` for backwards compatibility with
    # legacy PyPI imports (`from pynini.lib import ...`).
    foreach(SUBPKG lib export examples)
      install(
        FILES ${WHEEL_STAGE_DIR}/empty_init.py
        DESTINATION ${PYNINI_PYTHON_INSTALL_DIR}/opengrm/pynini/${SUBPKG}
        RENAME __init__.py
      )
      install(FILES ${WHEEL_STAGE_DIR}/py.typed
              DESTINATION ${PYNINI_PYTHON_INSTALL_DIR}/opengrm/pynini/${SUBPKG}
      )
      file(
        GLOB SUBPKG_MODULES
        LIST_DIRECTORIES false
        "${CMAKE_CURRENT_SOURCE_DIR}/${SUBPKG}/*.py"
      )
      list(
        FILTER
        SUBPKG_MODULES
        EXCLUDE
        REGEX
        "_test\\.py$"
      )
      install(FILES ${SUBPKG_MODULES}
              DESTINATION ${PYNINI_PYTHON_INSTALL_DIR}/opengrm/pynini/${SUBPKG}
      )

      file(MAKE_DIRECTORY ${WHEEL_STAGE_DIR}/pynini/${SUBPKG})
      file(WRITE ${WHEEL_STAGE_DIR}/pynini/${SUBPKG}/__init__.py "")
      foreach(MOD_PATH ${SUBPKG_MODULES})
        get_filename_component(MOD_NAME ${MOD_PATH} NAME_WE)
        file(WRITE ${WHEEL_STAGE_DIR}/pynini/${SUBPKG}/${MOD_NAME}.py
             "from opengrm.pynini.${SUBPKG}.${MOD_NAME} import *\n"
        )
      endforeach()
      install(DIRECTORY ${WHEEL_STAGE_DIR}/pynini/${SUBPKG}/
              DESTINATION ${PYNINI_PYTHON_INSTALL_DIR}/pynini/${SUBPKG}
      )
      install(FILES ${WHEEL_STAGE_DIR}/py.typed
              DESTINATION ${PYNINI_PYTHON_INSTALL_DIR}/pynini/${SUBPKG}
      )
    endforeach()

    # Section 2: Installs the top-level `pynini` compatibility package (`import
    # pynini`) that re-exports all symbols and `__version__` from
    # `opengrm.pynini.pynini`.
    file(MAKE_DIRECTORY ${WHEEL_STAGE_DIR}/pynini)
    file(
      WRITE ${WHEEL_STAGE_DIR}/pynini/__init__.py
      "from opengrm.pynini.pynini import *\nfrom opengrm.pynini.pynini import __version__\n"
    )
    install(FILES ${WHEEL_STAGE_DIR}/pynini/__init__.py
                  ${WHEEL_STAGE_DIR}/py.typed
            DESTINATION ${PYNINI_PYTHON_INSTALL_DIR}/pynini
    )

    # Section 3: Installs bundled OpenFst Python bindings under both the
    # monorepo namespace (`from openfst import pywrapfst`) and the legacy
    # top-level PyPI namespace (`import pywrapfst`).
    if(TARGET pywrapfst)
      install(TARGETS pywrapfst
              DESTINATION ${PYNINI_PYTHON_INSTALL_DIR}/openfst/extensions/python
      )
      install(
        FILES ${OPENFST_TEST_ENV}/openfst/__init__.py
              ${OPENFST_TEST_ENV}/openfst/pywrapfst.py
              ${WHEEL_STAGE_DIR}/py.typed
        DESTINATION ${PYNINI_PYTHON_INSTALL_DIR}/openfst
      )
      install(FILES ${OPENFST_TEST_ENV}/openfst/extensions/__init__.py
              DESTINATION ${PYNINI_PYTHON_INSTALL_DIR}/openfst/extensions
      )
      install(FILES ${OPENFST_TEST_ENV}/openfst/extensions/python/__init__.py
                    ${WHEEL_STAGE_DIR}/py.typed
              DESTINATION ${PYNINI_PYTHON_INSTALL_DIR}/openfst/extensions/python
      )

      file(MAKE_DIRECTORY ${WHEEL_STAGE_DIR}/pywrapfst)
      file(WRITE ${WHEEL_STAGE_DIR}/pywrapfst/__init__.py
           "from openfst.extensions.python.pywrapfst import *\n"
      )
      install(FILES ${WHEEL_STAGE_DIR}/pywrapfst/__init__.py
                    ${WHEEL_STAGE_DIR}/py.typed
              DESTINATION ${PYNINI_PYTHON_INSTALL_DIR}/pywrapfst
      )
      if(openfst_SOURCE_DIR
         AND EXISTS
             ${openfst_SOURCE_DIR}/openfst/extensions/python/pywrapfst.pyi
      )
        install(
          FILES ${openfst_SOURCE_DIR}/openfst/extensions/python/pywrapfst.pyi
          DESTINATION ${PYNINI_PYTHON_INSTALL_DIR}/openfst/extensions/python
        )
      endif()
    endif()
  endif()

  if(OPENGRM_BUILD_TESTS)
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

    set(TEST_PYTHONPATH
        "${CMAKE_CURRENT_BINARY_DIR}${PATH_SEP}${OPENFST_TEST_ENV}"
    )

    # Tests
    add_test(NAME pynini_test
             COMMAND ${Python3_EXECUTABLE}
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
  endif()

else()
  message(STATUS "Python3 or Cython not found. Skipping Pynini build.")
endif()
