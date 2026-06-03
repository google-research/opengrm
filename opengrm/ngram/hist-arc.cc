// Copyright 2026 The OpenGrm Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Source for histogram arc shared object.

#include "opengrm/ngram/hist-arc.h"

#include "openfst/lib/const-fst.h"
#include "openfst/lib/register.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/fstscript.h"
#include "openfst/script/weight-class.h"

namespace fst {
namespace script {

using ::ngram::HistogramArc;

REGISTER_FST(ConstFst, HistogramArc);
REGISTER_FST(VectorFst, HistogramArc);

REGISTER_FST_CLASSES(HistogramArc);
REGISTER_FST_OPERATIONS(HistogramArc);
REGISTER_FST_WEIGHT(HistogramArc::Weight);

}  // namespace script
}  // namespace fst
