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

# 1. Compile FST 1 and FST 2 using sfstngramread
"${BIN}/sfstngramread" \
  "${TESTDATA}/earnest.arpa" \
  "${TEST_TMPDIR}/fst1.fst"

"${BIN}/sfstngramread" \
  "${TESTDATA}/earnest.arpa" \
  "${TEST_TMPDIR}/fst2.fst"

# 2. Merge them using sfstmerge (method=linear)
"${BIN}/sfstmerge" \
  --method=linear \
  --alpha=0.5 \
  --beta=0.5 \
  "${TEST_TMPDIR}/fst1.fst" \
  "${TEST_TMPDIR}/fst2.fst" \
  "${TEST_TMPDIR}/merged.fst"

# 3. Merge them using sfstmerge (method=bayes)
"${BIN}/sfstmerge" \
  --method=bayes \
  --alpha=0.5 \
  --beta=0.5 \
  "${TEST_TMPDIR}/fst1.fst" \
  "${TEST_TMPDIR}/fst2.fst" \
  "${TEST_TMPDIR}/merged_bayes.fst"

# Verify output existence
if [[ ! -s "${TEST_TMPDIR}/merged.fst" || ! -s "${TEST_TMPDIR}/merged_bayes.fst" ]]; then
  echo "Error: Merged output files are empty or do not exist"
  exit 1
fi

echo "PASS"
