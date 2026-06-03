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
# Tests the command line binary ngramcount.

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

compile_test_far() {
  compile_test_fst "${1}"
  "${TEST_SRCDIR}/openfst+/openfst/extensions/far/farcreate" \
    "${TEST_TMPDIR}/${1}.ref" \
    "${TEST_TMPDIR}/${1}.far"
}

"${TEST_SRCDIR}/openfst+/openfst/extensions/far/farcompilestrings" \
  --fst_type=compact \
  --symbols="${TESTDATA}/earnest.sym" \
  --keep_symbols \
  "${TESTDATA}/earnest.txt" \
  "${TEST_TMPDIR}/earnest.far"

compile_test_fst earnest.cnts
# Counting from a FAR of string FSTs.
"${BIN}/ngramcount" \
  --order=5 \
  "${TEST_TMPDIR}/earnest.far" \
  "${TEST_TMPDIR}/earnest.cnts"
"${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
  "${TEST_TMPDIR}/earnest.cnts.ref" \
  "${TEST_TMPDIR}/earnest.cnts"

compile_test_far earnest.fst
compile_test_fst earnest-fst.cnts
# Counting from an FST representing a union of paths.
"${BIN}/ngramcount" \
  --order=5 \
  "${TEST_TMPDIR}/earnest.fst.far" \
  "${TEST_TMPDIR}/earnest.cnts"
"${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
  "${TEST_TMPDIR}/earnest-fst.cnts.ref" \
  "${TEST_TMPDIR}/earnest.cnts"

compile_test_far earnest.det
compile_test_fst earnest-det.cnts
# Counting from the deterministic "tree" FST representing the corpus.
"${BIN}/ngramcount" \
  --order=5 \
  "${TEST_TMPDIR}/earnest.det.far" \
  "${TEST_TMPDIR}/earnest-det.cnts"
"${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
  "${TEST_TMPDIR}/earnest-det.cnts.ref" \
  "${TEST_TMPDIR}/earnest-det.cnts"

compile_test_far earnest.min
compile_test_fst earnest-min.cnts
# Counting from the minimal deterministic FST representing the corpus.
"${BIN}/ngramcount" \
  --order=5 \
  "${TEST_TMPDIR}/earnest.min.far" \
  "${TEST_TMPDIR}/earnest-min.cnts"
"${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
  "${TEST_TMPDIR}/earnest-min.cnts.ref" \
  "${TEST_TMPDIR}/earnest-min.cnts"

compile_test_fst earnest.cnt_of_cnts
# Counting from counts.
"${BIN}/ngramcount" \
  --method=count_of_counts \
  "${TEST_TMPDIR}/earnest.cnts.ref" \
  "${TEST_TMPDIR}/earnest.cnt_of_cnts"
"${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
  "${TEST_TMPDIR}/earnest.cnt_of_cnts.ref" \
  "${TEST_TMPDIR}/earnest.cnt_of_cnts"
