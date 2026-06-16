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

"${TEST_SRCDIR}/openfst+/openfst/extensions/far/farcompilestrings" \
  --fst_type=compact \
  --symbols="${TESTDATA}/earnest.sym" \
  --keep_symbols \
  "${TESTDATA}/earnest.txt" \
  "${TEST_TMPDIR}/earnest.far"

"${BIN}/sfstngramcount" \
  --order=3 \
  "${TEST_TMPDIR}/earnest.far" \
  "${TEST_TMPDIR}/earnest.cnts"

"${BIN}/sfstsmooth" \
  --method=witten_bell \
  "${TEST_TMPDIR}/earnest.cnts" \
  "${TEST_TMPDIR}/earnest.mod"

if [ $# -ne 1 ]; then
  echo "Usage: $0 <method>"
  exit 1
fi
method=$1
echo "Testing shrinking method: ${method}"

case "${method}" in
  stolcke) param="--theta=0.00015" ;;
  seymore) param="--theta=4" ;;
  count_prune) param="--count_pattern=3+:2" ;;
  list_prune) param="" ;;
esac

if [ "${method}" == "list_prune" ]; then
  echo "the" > "${TEST_TMPDIR}/list.txt"

  "${BIN}/sfstshrink" \
    --method=list_prune \
    --list_file="${TEST_TMPDIR}/list.txt" \
    "${TEST_TMPDIR}/earnest.mod" \
    "${TEST_TMPDIR}/earnest-list.pru"

  if [[ ! -s "${TEST_TMPDIR}/earnest-list.pru" ]]; then
    echo "Error: Output file for method list_prune is empty or does not exist"
    exit 1
  fi

  "${BIN}/sfstinfo" "${TEST_TMPDIR}/earnest-list.pru" > /dev/null
else
  "${BIN}/sfstshrink" \
    --method="${method}" \
    ${param} \
    "${TEST_TMPDIR}/earnest.mod" \
    "${TEST_TMPDIR}/earnest-${method}.pru"

  if [[ ! -s "${TEST_TMPDIR}/earnest-${method}.pru" ]]; then
    echo "Error: Output file for method ${method} is empty or does not exist"
    exit 1
  fi

  "${BIN}/sfstinfo" "${TEST_TMPDIR}/earnest-${method}.pru" > /dev/null
fi

echo "PASS"
