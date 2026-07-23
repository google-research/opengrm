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

# Run sfstngramread.
"${BIN}/sfstngramread" \
  "${TESTDATA}/earnest.arpa" \
  "${TEST_TMPDIR}/earnest.fst"

# Verify output exists and is not empty.
if [[ ! -s "${TEST_TMPDIR}/earnest.fst" ]]; then
  echo "Error: Output file is empty or does not exist"
  exit 1
fi

# Run sfstinfo to verify it can read it.
"${BIN}/sfstinfo" "${TEST_TMPDIR}/earnest.fst" > /dev/null

echo "PASS"
