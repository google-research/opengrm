# Dockerfile
# Pierre-André Noël, May 12th 2020
# Copyright © Element AI Inc. All rights reserved.
# Copyright 2026 The OpenGrm Authors.
# Apache License, Version 2.0
#
# This builds `manylinux_2_28_x86_64` Python wheels for `pynini` (and bundled
# `pywrapfst`), wrapping all its C++ dependencies (Abseil, OpenFst, OpenGrm).
#
# This Dockerfile uses multi-stage builds; for more information, see:
# https://docs.docker.com/develop/develop-images/multistage-build/
#
# To build wheels for all supported Python versions (3.11-3.15) and run Pynini's
# test suite from the root of the OpenGrm repository, run:
#
#     docker build -f opengrm/pynini/Dockerfile --target=run-tests \
#         -t build-pynini-wheels .
#
# To build and test a single Python runtime shard (e.g., CPython 3.12), run:
#
#     docker build -f opengrm/pynini/Dockerfile --target=run-tests \
#         --build-arg PYTHON_TAGS=cp312-cp312 -t build-pynini-wheels .
#
# To extract the validated wheels directly to `./wheelhouse` using BuildKit:
#
#     docker buildx build -f opengrm/pynini/Dockerfile --target=export-wheels \
#         --output type=local,dest=./wheelhouse .
#
# Or from a built `run-tests` image:
#
#     docker run --rm -v "$(pwd)":/io build-pynini-wheels cp -r /wheelhouse /io

# ******************************************************
# *** All the following images are based on this one ***
# ******************************************************
FROM quay.io/pypa/manylinux_2_28_x86_64:latest AS common

# Prunes EOL Python interpreters (<= 3.10) and experimental free-threaded builds.
RUN rm -rf \
    /opt/python/cp38-* \
    /opt/python/cp39-* \
    /opt/python/cp310-* \
    /opt/python/pp38-* \
    /opt/python/pp39-* \
    /opt/python/pp310-* \
    /opt/python/*t

# ***********************************************************************
# *** Image providing all the requirements for building Pynini wheels ***
# ***********************************************************************
FROM common AS wheel-building-env

# Installs ccache and ninja for fast C++ compilation across Python shards.
RUN yum install -y ccache ninja-build && yum clean all

ENV CCACHE_DIR="/ccache" \
    CCACHE_BASEDIR="/src" \
    CCACHE_NOHASHDIR="true" \
    CMAKE_C_COMPILER_LAUNCHER="ccache" \
    CMAKE_CXX_COMPILER_LAUNCHER="ccache" \
    CMAKE_GENERATOR="Ninja"

# Copies the OpenGrm repository source tree.
COPY . /src

# Pre-fetches C++ dependencies (abseil-cpp and openfst) into /deps and compiles
# all Python-independent C++ translation units once into /ccache so per-Python
# wheel builds do not hit the network or recompile C++ libraries.
RUN cmake -S /src -B /tmp/cpp-warmup -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=ON \
        -DOPENGRM_BUILD_TESTS=OFF \
        -DOPENGRM_ENABLE_BAUMWELCH=OFF \
        -DOPENGRM_ENABLE_BIN=OFF \
        -DOPENGRM_ENABLE_INSTALL=OFF \
        -DOPENGRM_ENABLE_NGRAM=OFF \
        -DOPENGRM_ENABLE_PYNINI=OFF \
        -DOPENGRM_ENABLE_SFST=OFF \
        -DOPENGRM_ENABLE_THRAX=OFF \
        -DOPENFST_ENABLE_GRM=ON \
        -DOPENFST_ENABLE_PDT=ON \
        -DOPENFST_ENABLE_MPDT=ON \
        -DOPENFST_ENABLE_FAR=ON \
        -DFETCHCONTENT_BASE_DIR=/deps \
    && cmake --build /tmp/cpp-warmup \
    && rm -rf /tmp/cpp-warmup /deps/*-build /deps/*-subbuild

ENV CMAKE_ARGS="-DFETCHCONTENT_SOURCE_DIR_ABSEIL-CPP=/deps/abseil-cpp-src -DFETCHCONTENT_SOURCE_DIR_OPENFST=/deps/openfst-src"

# **********************************************************
# *** Image making pynini wheels (placed in /wheelhouse) ***
# **********************************************************
FROM wheel-building-env AS build-wheels

# Specifies space-separated glob(s) of /opt/python tags to build (e.g.,
# "cp312-cp312" in sharded CI).
ARG PYTHON_TAGS="*"

# Compiles the wheels to a temporary directory and repairs them with auditwheel.
RUN mkdir -p /tmp/wheelhouse /wheelhouse \
    && found=0 \
    && for TAG in ${PYTHON_TAGS}; do \
        for PYBIN in /opt/python/${TAG}/bin; do \
            [ -x "${PYBIN}/pip" ] || continue; \
            found=1; \
            "${PYBIN}/pip" install --upgrade \
                pip "scikit-build-core>=0.10" "Cython>=3.0.6" || exit 1; \
            "${PYBIN}/pip" wheel --no-build-isolation --no-deps -v /src \
                -w /tmp/wheelhouse/ || exit 1; \
        done; \
    done \
    && [ "${found}" -eq 1 ] \
    && for WHL in /tmp/wheelhouse/pynini*.whl; do \
        auditwheel repair --plat manylinux_2_28_x86_64 "${WHL}" \
            -w /wheelhouse/ || exit 1; \
    done \
    && rm -rf /tmp/wheelhouse /src/build

# *******************************************************
# *** Installs wheels in a fresh (OpenFst-free) image ***
# *******************************************************
FROM common AS install-pynini-from-wheels

ARG PYTHON_TAGS="*"

# Grabs the repaired wheels (and only the wheels) from the previous stage.
COPY --from=build-wheels /wheelhouse /wheelhouse

# Pre-installs the pure-Python `absl-py` runtime dependency from PyPI, then
# installs `pynini` strictly from `/wheelhouse` (`--no-index`) so `pip` cannot
# fall back to PyPI for `pynini` itself.
RUN found=0 \
    && for TAG in ${PYTHON_TAGS}; do \
        for PYBIN in /opt/python/${TAG}/bin; do \
            [ -x "${PYBIN}/pip" ] || continue; \
            found=1; \
            "${PYBIN}/pip" install absl-py || exit 1; \
            "${PYBIN}/pip" install pynini --no-index -f /wheelhouse || exit 1; \
        done; \
    done \
    && [ "${found}" -eq 1 ]

# ***************************
# *** Runs Pynini's tests ***
# ***************************
FROM install-pynini-from-wheels AS run-tests

ARG PYTHON_TAGS="*"

# Copies Pynini's test scripts and testdata into isolated directories.
COPY --from=wheel-building-env /src/opengrm/pynini /tests
COPY --from=wheel-building-env \
    /deps/openfst-src/openfst/test/testdata \
    /testrunfiles/openfst/test/testdata
COPY --from=wheel-building-env \
    /deps/openfst-src/openfst/extensions/far/testdata \
    /testrunfiles/openfst/extensions/far/testdata
COPY --from=wheel-building-env \
    /deps/openfst-src/openfst/extensions/mpdt/testdata \
    /testrunfiles/openfst/extensions/mpdt/testdata
COPY --from=wheel-building-env \
    /deps/openfst-src/openfst/extensions/pdt/testdata \
    /testrunfiles/openfst/extensions/pdt/testdata
COPY --from=wheel-building-env \
    /src/opengrm/string/testdata \
    /testrunfiles/opengrm/string/testdata
COPY --from=wheel-building-env \
    /src/opengrm/operators/testdata \
    /testrunfiles/opengrm/operators/testdata
COPY --from=wheel-building-env \
    /src/opengrm/pynini/examples/testdata \
    /testrunfiles/opengrm/pynini/examples/testdata

ENV TEST_SRCDIR="/testrunfiles"

# Runs namespace import smoke tests and Pynini's unit tests from an empty
# working directory so imports come exclusively from the installed wheel.
# Note: `chatspeak_model_test.py` is skipped because its `earnest.fst` LM
# fixture is generated at build time by `sfst` CLI binaries in Bazel.
RUN mkdir -p /tmp/test_workdir && cd /tmp/test_workdir \
    && for TAG in ${PYTHON_TAGS}; do \
        for PYBIN in /opt/python/${TAG}/bin; do \
            [ -x "${PYBIN}/python" ] || continue; \
            "${PYBIN}/python" -c "\
import importlib.metadata, os; \
import pynini; \
import pywrapfst; \
from pynini.lib import byte, edit_transducer, features, paradigms, pynutil, rewrite, rule_cascade, stringfile, tagger, utf8; \
from pynini.export import export, grm, multi_grm; \
from pynini.examples import case, chatspeak, dates, g2p, numbers, plurals, t9, weather; \
from opengrm.pynini import pynini as grm_pynini; \
from openfst import pywrapfst as fst_pywrapfst; \
assert pynini.__version__ == importlib.metadata.version('pynini'); \
assert grm_pynini.__version__ == pynini.__version__; \
assert pynini.Fst is grm_pynini.Fst; \
assert pywrapfst.Fst is fst_pywrapfst.Fst; \
assert os.path.exists(os.path.join(os.path.dirname(grm_pynini.__file__), 'pynini.pyi'))" || exit 1; \
            for TEST in \
                /tests/pynini_test.py \
                /tests/runfiles_test.py \
                /tests/lib/*_test.py \
                /tests/export/*_test.py \
                /tests/examples/*_test.py; do \
                if [[ "${TEST}" == "/tests/examples/chatspeak_model_test.py" ]]; then \
                    continue; \
                fi; \
                "${PYBIN}/python" "${TEST}" || exit 1; \
            done; \
        done; \
    done

# **************************************************************************
# *** Minimal artifact export stage (requires `run-tests` to pass first) ***
# **************************************************************************
FROM scratch AS export-wheels

COPY --from=run-tests /wheelhouse/ /
