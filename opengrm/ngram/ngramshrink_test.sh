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
# Tests the command line binary ngramshrink.

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
for METHOD in count_prune relative_entropy seymore; do
  case "${METHOD}" in
    count_prune) PARAM="--count_pattern=3+:2" ;;
    relative_entropy) PARAM="--theta=.00015" ;;
    seymore) PARAM="--theta=4" ;;
  esac

  compile_test_fst "earnest-${METHOD}.pru"
  "${BIN}/ngramshrink" \
    --method="${METHOD}" \
    --check_consistency \
    "${PARAM}" \
    "${TEST_TMPDIR}/earnest-witten_bell.mod.ref" \
    "${TEST_TMPDIR}/${METHOD}.pru"

  "${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
    "${TEST_TMPDIR}/earnest-${METHOD}.pru.ref" \
    "${TEST_TMPDIR}/${METHOD}.pru"
done

for METHOD in relative_entropy seymore; do
  case "${METHOD}" in
    relative_entropy) TARGET=5897 ;;
    seymore) TARGET=5276 ;;
  esac

  "${BIN}/ngramshrink" \
    --method="${METHOD}" \
    --check_consistency \
    --target_number_of_ngrams="${TARGET}" \
    "${TEST_TMPDIR}/earnest-witten_bell.mod.ref" \
    "${TEST_TMPDIR}/${METHOD}.target.pru"

  "${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
    "${TEST_TMPDIR}/earnest-${METHOD}.pru.ref" \
    "${TEST_TMPDIR}/${METHOD}.target.pru"
done
