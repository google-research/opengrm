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
# Unit test for fstcross.

set -eou pipefail

source "${TEST_SRCDIR}/openfst+/openfst/bin/setup.sh" || exit

readonly RUNPATH="${TEST_SRCDIR}/${TEST_WORKSPACE}/opengrm/operators"
readonly SINK="${TEST_TMPDIR}/$$.tmp"

"${RUNPATH}/fstcross" \
  "${RUNPATH}/testdata/upper.fst" \
  "${RUNPATH}/testdata/lower.fst" "${SINK}"
"${TEST_SRCDIR}/openfst+/openfst/bin/fstequal" -v=1 \
  "${SINK}" "${RUNPATH}/testdata/xprod.fst"
