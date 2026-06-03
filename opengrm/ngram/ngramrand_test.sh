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

# Random test of ngram functionality.
#
# Can set environment variable NGRAMRANDTRIALS, otherwise defaults to 0 trials.
# When called with an integer argument, will run that many trials, otherwise
# the default number as explained above.

set -eou pipefail

source "${TEST_SRCDIR}/openfst+/openfst/bin/setup.sh" || exit

readonly BIN="${TEST_SRCDIR}/${TEST_WORKSPACE}/opengrm/ngram"
readonly TEST_TMPDIR="${TEST_TMPDIR:-$(mktemp -d)}"

readonly DEFTRIALS=${NGRAMRANDTRIALS:-0}
readonly TRIALS=${1:-$DEFTRIALS}
readonly VARFILE="${TEST_TMPDIR}/ngramrandtest.vars"

i=0
while [[ $i -lt $TRIALS ]]; do
  : $((i+=1))
  rm -rf "${TEST_TMPDIR}/."*
  # Runs random test, outputs various count and model files and variables.
  "${BIN}/ngramrandtest" --directory="${TEST_TMPDIR}" --vars="${VARFILE}"
  # Reads in variables from rand test (SEED, ORDER ...).
  . "${VARFILE}"
  "${BIN}/ngramdistrand" "$SEED" "$ORDER"
  rm -rf "${TEST_TMPDIR}/${SEED}."* "${TEST_TMPDIR}/."*
done
