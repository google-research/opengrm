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
# Tests the command line binary ngrammerge.

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

compile_test_fst earnest-absolute.mod
compile_test_fst earnest-seymore.pru
compile_test_fst earnest.mrg
"${BIN}/ngrammerge" \
  --check_consistency \
  --method=count_merge \
  "${TEST_TMPDIR}/earnest-absolute.mod.ref" \
  "${TEST_TMPDIR}/earnest-seymore.pru.ref" \
  "${TEST_TMPDIR}/earnest.mrg"

"${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
  "${TEST_TMPDIR}/earnest.mrg.ref" \
  "${TEST_TMPDIR}/earnest.mrg"

compile_test_fst earnest.mrg.norm
"${BIN}/ngrammerge" \
  --check_consistency \
  --method=count_merge \
  --normalize \
  "${TEST_TMPDIR}/earnest-absolute.mod.ref" \
  "${TEST_TMPDIR}/earnest-seymore.pru.ref" \
  "${TEST_TMPDIR}/earnest.mrg.norm"

"${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
  "${TEST_TMPDIR}/earnest.mrg.norm.ref" \
  "${TEST_TMPDIR}/earnest.mrg.norm"

compile_test_fst earnest.mrg.smooth
"${BIN}/ngrammerge" \
  --check_consistency \
  --method=model_merge \
  "${TEST_TMPDIR}/earnest-absolute.mod.ref" \
  "${TEST_TMPDIR}/earnest-seymore.pru.ref" \
  "${TEST_TMPDIR}/earnest.mrg.smooth"

"${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
  "${TEST_TMPDIR}/earnest.mrg.smooth.ref" \
  "${TEST_TMPDIR}/earnest.mrg.smooth"

compile_test_fst earnest.mrg.smooth.norm
"${BIN}/ngrammerge" \
  --check_consistency \
  --method=model_merge \
  --normalize \
  "${TEST_TMPDIR}/earnest-absolute.mod.ref" \
  "${TEST_TMPDIR}/earnest-seymore.pru.ref" \
  "${TEST_TMPDIR}/earnest.mrg.smooth.norm"

"${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
  "${TEST_TMPDIR}/earnest.mrg.smooth.norm.ref" \
  "${TEST_TMPDIR}/earnest.mrg.smooth.norm"
