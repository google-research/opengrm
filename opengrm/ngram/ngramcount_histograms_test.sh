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
# Tests that ngramcount computes correct histogram counts.

set -eou pipefail

source "${TEST_SRCDIR}/openfst+/openfst/bin/setup.sh" || exit

readonly BIN="${TEST_SRCDIR}/${TEST_WORKSPACE}/opengrm/ngram"
readonly TESTDATA="${TEST_SRCDIR}/${TEST_WORKSPACE}/opengrm/ngram/testdata"
readonly TEST_TMPDIR="${TEST_TMPDIR:-$(mktemp -d)}"

compile_test_fst() {
  "${TEST_SRCDIR}/openfst+/openfst/bin/fstcompile" \
    --isymbols="${TESTDATA}/${2}.sym" \
    --osymbols="${TESTDATA}/${2}.sym" \
    --keep_isymbols \
    --keep_osymbols \
    --keep_state_numbering \
    "${TESTDATA}/${1}.txt" \
    "${TEST_TMPDIR}/${1}.ref"
}

compile_test_fst single_fst ab
"${TEST_SRCDIR}/openfst+/openfst/extensions/far/farcreate" \
  "${TEST_TMPDIR}/single_fst.ref" \
  "${TEST_TMPDIR}/single_fst.far"
"${BIN}/ngramhisttest" \
  --ifile="${TESTDATA}/single_fst_ref.txt" \
  --syms="${TESTDATA}/ab.sym" \
  --ofile="${TEST_TMPDIR}/single_fst_ref.ref"
"${BIN}/ngramcount" \
  --order=3 \
  --method=histograms \
  "${TEST_TMPDIR}/single_fst.far" \
  "${TEST_TMPDIR}/single_fst.cnts"
"${BIN}/ngramhisttest" \
  --ifile="${TEST_TMPDIR}/single_fst.cnts" \
  --cfile="${TEST_TMPDIR}/single_fst_ref.ref"

"${BIN}/ngramhisttest" \
  --ifile="${TESTDATA}/hist.ref.txt" \
  --syms="${TESTDATA}/ab.sym" \
  --ofile="${TEST_TMPDIR}/hist.ref.ref"
compile_test_fst fst1.hist ab
compile_test_fst fst2.hist ab
"${TEST_SRCDIR}/openfst+/openfst/extensions/far/farcreate" \
  "${TEST_TMPDIR}/fst1.hist.ref" \
  "${TEST_TMPDIR}/fst2.hist.ref" \
  "${TEST_TMPDIR}/test.far"
"${BIN}/ngramcount" \
   --order=2 \
   --method=histograms \
  "${TEST_TMPDIR}/test.far" \
  "${TEST_TMPDIR}/test.cnts"
"${BIN}/ngramhisttest" \
  --ifile="${TEST_TMPDIR}/test.cnts" \
  --cfile="${TEST_TMPDIR}/hist.ref.ref"
