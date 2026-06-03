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
# Tests the command line binary ngrammake.

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

# Default method.
compile_test_fst earnest.cnts
compile_test_fst earnest.mod
"${BIN}/ngrammake" \
  --check_consistency \
  "${TEST_TMPDIR}/earnest.cnts.ref" \
  "${TEST_TMPDIR}/earnest.mod"
"${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
  "${TEST_TMPDIR}/earnest.mod.ref" \
  "${TEST_TMPDIR}/earnest.mod"

# Specified methods.
for METHOD in absolute katz witten_bell kneser_ney unsmoothed; do
  compile_test_fst "earnest-${METHOD}.mod"
  "${BIN}/ngrammake" \
    --method="${METHOD}" \
    --check_consistency \
    "${TEST_TMPDIR}/earnest.cnts.ref" \
    "${TEST_TMPDIR}/earnest-${METHOD}.mod"
  "${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
    "${TEST_TMPDIR}/earnest-${METHOD}.mod.ref" \
    "${TEST_TMPDIR}/earnest-${METHOD}.mod"
done

# Fractional counting.
"${TEST_SRCDIR}/openfst+/openfst/extensions/far/farcompilestrings" \
  --fst_type=compact \
  --symbols="${TESTDATA}/earnest.sym" \
  --keep_symbols \
  "${TESTDATA}/earnest.txt" \
  "${TEST_TMPDIR}/earnest.far"

"${BIN}/ngramcount" \
  --method=histograms \
  "${TEST_TMPDIR}/earnest.far" \
  "${TEST_TMPDIR}/earnest.hsts"

"${BIN}/ngramcount" \
  --method=counts \
  "${TEST_TMPDIR}/earnest.far" \
  "${TEST_TMPDIR}/earnest.cnts"

"${BIN}/ngrammake" \
  --method=katz_frac \
  --check_consistency \
  "${TEST_TMPDIR}/earnest.hsts" \
  "${TEST_TMPDIR}/earnest-katz_frac.mod"

"${BIN}/ngrammake" \
  --method=katz \
  --bins=4 \
  --check_consistency \
  "${TEST_TMPDIR}/earnest.cnts" \
  "${TEST_TMPDIR}/earnest-katz_frac.mod.ref"

"${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
  "${TEST_TMPDIR}/earnest-katz_frac.mod.ref" \
  "${TEST_TMPDIR}/earnest-katz_frac.mod"
