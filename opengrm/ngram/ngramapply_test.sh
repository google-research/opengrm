#!/bin/bash

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
#
# Tests the command line binary ngramapply.

set -eou pipefail

source "${TEST_SRCDIR}/openfst+/openfst/bin/setup.sh" || exit

readonly BIN="${TEST_SRCDIR}/${TEST_WORKSPACE}/opengrm/ngram"
readonly TESTDATA="${TEST_SRCDIR}/${TEST_WORKSPACE}/opengrm/ngram/testdata"
readonly TEST_TMPDIR="${TEST_TMPDIR:-$(mktemp -d)}"

compile_test_fst() {
  "${TEST_SRCDIR}/openfst+/openfst/bin/fstcompile" \
    --isymbols="${TESTDATA}/${1}.sym" \
    --osymbols="${TESTDATA}/${1}.sym" \
    --keep_isymbols \
    --keep_osymbols \
    --keep_state_numbering \
    "${TESTDATA}/${1}.txt" \
    "${TEST_TMPDIR}/${1}.ref"
}

compile_test_fst earnest-witten_bell.mod

# Create FARs.
"${TEST_SRCDIR}/openfst+/openfst/extensions/far/farcompilestrings" \
  --stderrthreshold=0 \
  --fst_type=compact \
  --key_prefix="FST" \
  --generate_keys=4 \
  --symbols="${TESTDATA}/earnest.randgen.apply.sym" \
  --keep_symbols \
  "${TESTDATA}/earnest.randgen.txt" \
  "${TEST_TMPDIR}/earnest.far"
tar -xzf \
  "${TESTDATA}/earnest.randgen.apply.FSTtxt.tgz" \
  -C \
  "${TEST_TMPDIR}"
"${TEST_SRCDIR}/openfst+/openfst/bin/fstcompile" \
  --stderrthreshold=0 \
  --isymbols="${TESTDATA}/earnest.randgen.sym" \
  --osymbols="${TESTDATA}/earnest.randgen.sym" \
  --keep_state_numbering \
  "${TEST_TMPDIR}/FST0001.txt" \
  "${TEST_TMPDIR}/FST0001"
ls "${TEST_TMPDIR}/FST"????.txt \
  | grep -v FST0001 \
  | sed 's/.txt$//g' \
  | while read I; do
    "${TEST_SRCDIR}/openfst+/openfst/bin/fstcompile" \
    --stderrthreshold=0 \
    "${I}.txt" \
    "${I}"
  done
"${TEST_SRCDIR}/openfst+/openfst/extensions/far/farcreate" \
    --stderrthreshold=0 \
    "${TEST_TMPDIR}/FST"???? "${TEST_TMPDIR}/earnest.apply.far.ref"

"${BIN}/ngramapply" \
  --stderrthreshold=0 \
  "${TEST_TMPDIR}/earnest-witten_bell.mod.ref" \
  "${TEST_TMPDIR}/earnest.far" \
  "${TEST_TMPDIR}/earnest.apply.far"

"${TEST_SRCDIR}/openfst+/openfst/extensions/far/farequal" \
  --stderrthreshold=0 \
  "${TEST_TMPDIR}/earnest.apply.far.ref" \
  "${TEST_TMPDIR}/earnest.apply.far"
