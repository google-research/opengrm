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

readonly BIN="${TEST_SRCDIR}/${TEST_WORKSPACE}/opengrm/ngram"
readonly TESTDATA="${TEST_SRCDIR}/${TEST_WORKSPACE}/opengrm/ngram/testdata"
readonly TEST_TMPDIR="${TEST_TMPDIR:-$(mktemp -d)}"

readonly FRAC_DIST="${FRAC_DIST:-false}"
if [[ ${FRAC_DIST} = true ]]; then
  echo "FRAC"
  readonly DIST_BIN="${BIN}/ngramfractrain"
  readonly NODIST_DISCOUNT_BINS=4
else
  echo "NOFRAC"
  readonly DIST_BIN="${BIN}/ngramdisttrain"
  readonly NODIST_DISCOUNT_BINS=-1
fi
readonly NODIST_BIN="${BIN}/ngramdisttrain"

# Sentence to split data on.
readonly SPLIT_DATA=850

# Word ID to split context on.
readonly SPLIT_CNTX1=250
readonly SPLIT_CNTX2=500
readonly SPLIT_CNTX3=1000

# Sets up distributed data.
awk "NR<$SPLIT_DATA" "${TESTDATA}/earnest.txt" > "${TEST_TMPDIR}/earnest.txt1"
awk "NR>=$SPLIT_DATA" "${TESTDATA}/earnest.txt" > "${TEST_TMPDIR}/earnest.txt2"

# Sets up distributed contexts.
cat <<EOF > "${TEST_TMPDIR}/earnest.cntxs"
0 : $SPLIT_CNTX1
$SPLIT_CNTX1 : $SPLIT_CNTX2
$SPLIT_CNTX2 : $SPLIT_CNTX3
$SPLIT_CNTX3 : 2306
EOF

# Tests FST equality after assuring same ordering.
ngramequal() {
  "${BIN}/ngramsort" "$1" "${TEST_TMPDIR}/earnest.eq1"
  "${BIN}/ngramsort" "$2" "${TEST_TMPDIR}/earnest.eq2"
  "${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
    -v=1 \
    "${TEST_TMPDIR}/earnest.eq1" \
    "${TEST_TMPDIR}/earnest.eq2"
}

distributed_test() {
  # Non-distributed version.
  "${NODIST_BIN}" \
    --itype=text_sents \
    --symbols="${TESTDATA}/earnest.sym" \
    "${@:+"$@"}" \
    --bins="${NODIST_DISCOUNT_BINS}" \
    --ifile="${TESTDATA}/earnest.txt" \
    --ofile="${TEST_TMPDIR}/earnest.nodist"

  # Distributed version.
  "${DIST_BIN}" \
    --itype=text_sents \
    --symbols="${TESTDATA}/earnest.sym" \
    "${@:+"$@"}" \
    --contexts="${TEST_TMPDIR}/earnest.cntxs" \
    --merge_contexts \
    --ifile="${TEST_TMPDIR}/earnest.txt[12]" \
    --ofile="${TEST_TMPDIR}/earnest.dist"

  # Verifies non-distributed and distributed versions give the same result.
  ngramequal "${TEST_TMPDIR}/earnest.nodist" "${TEST_TMPDIR}/earnest.dist"
}
