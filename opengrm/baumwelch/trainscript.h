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

#ifndef OPENGRM_BAUMWELCH_TRAINSCRIPT_H_
#define OPENGRM_BAUMWELCH_TRAINSCRIPT_H_

#include <tuple>

#include "openfst/extensions/far/far-class.h"
#include "openfst/extensions/far/far.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/script/fst-class.h"
#include "opengrm/baumwelch/train.h"

namespace fst {
namespace script {

using BaumWelchTrainArgs =
    std::tuple<FarReaderClass&, FarReaderClass&, MutableFstClass*, bool,
               const TrainOptions&>;

template <class Arc>
void Train(BaumWelchTrainArgs* args) {
  FarReader<Arc>& input = *std::get<0>(*args).GetFarReader<Arc>();
  FarReader<Arc>& output = *std::get<1>(*args).GetFarReader<Arc>();
  MutableFst<Arc>* model = std::get<2>(*args)->GetMutableFst<Arc>();
  Train(input, output, model, std::get<3>(*args), std::get<4>(*args));
}

void Train(FarReaderClass& input, FarReaderClass& output,
           MutableFstClass* model, bool normalize_ilabel = true,
           const TrainOptions& opts = TrainOptions());

}  // namespace script
}  // namespace fst

#endif  // OPENGRM_BAUMWELCH_TRAINSCRIPT_H_
