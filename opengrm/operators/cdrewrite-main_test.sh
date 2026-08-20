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
# Unit test for fstcdrewrite.

set -eou pipefail

source "${TEST_SRCDIR}/openfst+/openfst/bin/setup.sh" || exit

readonly RUNPATH="${TEST_SRCDIR}/${TEST_WORKSPACE}/opengrm/operators"
readonly SINK="${TEST_TMPDIR}/$$.tmp"

"${RUNPATH}/fstcdrewrite" \
  --initial_boundary_marker=20 \
  --final_boundary_marker=21 \
  "${RUNPATH}/testdata/bXa.fst" \
  "${RUNPATH}/testdata/a_bos.fst" \
  "${RUNPATH}/testdata/b_eos.fst" \
  "${RUNPATH}/testdata/sigma.fst" \
  "${SINK}"

"${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" \
  -v=1 \
  "${RUNPATH}/testdata/sigma.fst" \
  "${SINK}"
