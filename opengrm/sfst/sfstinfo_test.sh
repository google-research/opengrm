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

set -eou pipefail

source "${TEST_SRCDIR}/openfst+/openfst/bin/setup.sh" || exit

readonly BIN="${TEST_SRCDIR}/${TEST_WORKSPACE}/opengrm/sfst"
readonly TESTDATA="${TEST_SRCDIR}/${TEST_WORKSPACE}/opengrm/ngram/testdata"
readonly TEST_TMPDIR="${TEST_TMPDIR:-$(mktemp -d)}"

compile_test_fst() {
  local name="$1"
  "${TEST_SRCDIR}/openfst+/openfst/bin/fstcompile" \
    --isymbols="${TESTDATA}/${name}.sym" \
    --osymbols="${TESTDATA}/${name}.sym" \
    --keep_isymbols \
    --keep_osymbols \
    --keep_state_numbering \
    "${TESTDATA}/${name}.txt" \
    "${TEST_TMPDIR}/${name}.ref"
}

compile_test_fst earnest.mod

"${BIN}/sfstinfo" "${TEST_TMPDIR}/earnest.mod.ref" > "${TEST_TMPDIR}/earnest.info"

# Verify basic fields using grep.
grep -q "# of states" "${TEST_TMPDIR}/earnest.info"
grep -q "# of arcs" "${TEST_TMPDIR}/earnest.info"
grep -q "initial state" "${TEST_TMPDIR}/earnest.info"

echo "PASS"
