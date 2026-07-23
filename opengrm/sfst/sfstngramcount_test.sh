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
readonly TESTDATA="${TEST_SRCDIR}/${TEST_WORKSPACE}/opengrm/sfst/testdata"
readonly TEST_TMPDIR="${TEST_TMPDIR:-$(mktemp -d)}"

"${TEST_SRCDIR}/openfst+/openfst/extensions/far/farcompilestrings" \
  --fst_type=compact \
  --symbols="${TESTDATA}/earnest.sym" \
  --keep_symbols \
  "${TESTDATA}/earnest.txt" \
  "${TEST_TMPDIR}/earnest.far"

"${BIN}/sfstngramcount" \
  --order=5 \
  "${TEST_TMPDIR}/earnest.far" \
  "${TEST_TMPDIR}/earnest.cnts"

# Verify output exists and is not empty.
if [[ ! -s "${TEST_TMPDIR}/earnest.cnts" ]]; then
  echo "Error: Output file is empty or does not exist"
  exit 1
fi

# Run sfstinfo to verify it can read it.
"${BIN}/sfstinfo" "${TEST_TMPDIR}/earnest.cnts" > /dev/null

# Compile a FAR without symbols.
"${TEST_SRCDIR}/openfst+/openfst/extensions/far/farcompilestrings" \
  --fst_type=compact \
  --token_type=utf8 \
  "${TESTDATA}/earnest.txt" \
  "${TEST_TMPDIR}/earnest_nosyms.far"

# Test 1: require_symbols=true (default) should fail.
if "${BIN}/sfstngramcount" \
  --order=5 \
  "${TEST_TMPDIR}/earnest_nosyms.far" \
  "${TEST_TMPDIR}/earnest_nosyms.cnts" 2>/dev/null; then
  echo "Error: sfstngramcount succeeded on FAR without symbols (expected failure)"
  exit 1
fi

# Test 2: require_symbols=false should succeed.
if ! "${BIN}/sfstngramcount" \
  --require_symbols=false \
  --order=5 \
  "${TEST_TMPDIR}/earnest_nosyms.far" \
  "${TEST_TMPDIR}/earnest_nosyms.cnts"; then
  echo "Error: sfstngramcount failed on FAR without symbols with require_symbols=false"
  exit 1
fi

# Verify output of Test 2 exists and is not empty.
if [[ ! -s "${TEST_TMPDIR}/earnest_nosyms.cnts" ]]; then
  echo "Error: Output file for require_symbols=false is empty or does not exist"
  exit 1
fi

echo "PASS"
