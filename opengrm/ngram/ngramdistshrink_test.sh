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
# Tests distributed training and pruning of language models.

source "${TEST_SRCDIR}/openfst+/openfst/bin/setup.sh" || exit
source "${TEST_SRCDIR}/${TEST_WORKSPACE}/opengrm/ngram/disttestsetup.sh" || exit

distributed_test \
    --otype=pruned_lm \
    --smooth_method=katz \
    --shrink_method=relative_entropy \
    --theta=.00015
distributed_test \
    --otype=pruned_lm \
    --smooth_method=katz \
    --shrink_method=seymore \
    --theta=4
