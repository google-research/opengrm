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
# Tests the command line binary ngramprint and ngramread.

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
"${BIN}/ngramprint" \
  --ARPA \
  --check_consistency \
  "${TEST_TMPDIR}/earnest-witten_bell.mod.ref" \
  "${TEST_TMPDIR}/earnest.arpa"

cmp "${TESTDATA}/earnest.arpa" "${TEST_TMPDIR}/earnest.arpa"

"${BIN}/ngramread" --ARPA "${TESTDATA}/earnest.arpa" "${TEST_TMPDIR}/earnest.arpa.mod"

"${BIN}/ngramprint" \
  --ARPA \
  --check_consistency \
  "${TEST_TMPDIR}/earnest.arpa.mod" \
  | "${BIN}/ngramread" \
  --ARPA \
  - \
  "${TEST_TMPDIR}/earnest.arpa.mod2"

"${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
  "${TEST_TMPDIR}/earnest.arpa.mod" \
  "${TEST_TMPDIR}/earnest.arpa.mod2"

compile_test_fst earnest.cnts
"${BIN}/ngramprint" \
  --check_consistency \
  "${TEST_TMPDIR}/earnest.cnts.ref" \
  "${TEST_TMPDIR}/earnest.cnt.print"

cmp "${TESTDATA}/earnest.cnt.print" "${TEST_TMPDIR}/earnest.cnt.print"

"${BIN}/ngramread" \
  --symbols="${TESTDATA}/earnest.sym" \
  "${TESTDATA}/earnest.cnt.print" \
  "${TEST_TMPDIR}/earnest.cnts"

"${BIN}/ngramprint" \
  --check_consistency \
  "${TEST_TMPDIR}/earnest.cnts" \
  | "${BIN}/ngramread" \
  --symbols="${TESTDATA}/earnest.sym" \
  - \
  "${TEST_TMPDIR}/earnest.cnts2"

"${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
  "${TEST_TMPDIR}/earnest.cnts" \
  "${TEST_TMPDIR}/earnest.cnts2"
