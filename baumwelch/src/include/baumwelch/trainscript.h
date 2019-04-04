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
//
// Copyright 2017 and onwards Google, Inc.

#ifndef BAUMWELCH_TRAINSCRIPT_H_
#define BAUMWELCH_TRAINSCRIPT_H_

#include <fst/extensions/far/far-class.h>
#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>
#include <baumwelch/getters.h>
#include <baumwelch/train.h>

namespace fst {
namespace script {

// 1: Pair construction.
using TrainBaumWelchInnerArgs1 =
    std::tuple<FarReaderClass *, FarReaderClass *, MutableFstClass *,
               ExpectationTableType, const BaumWelchTrainOptions &>;

using TrainBaumWelchArgs1 =
    WithReturnValue<bool, TrainBaumWelchInnerArgs1>;

template <class Arc>
void TrainBaumWelch(TrainBaumWelchArgs1 *args) {
  FarReader<Arc> *input = std::get<0>(args->args)->GetFarReader<Arc>();
  FarReader<Arc> *output = std::get<1>(args->args)->GetFarReader<Arc>();
  MutableFst<Arc> *channel = std::get<2>(args->args)->GetMutableFst<Arc>();
  args->retval = TrainBaumWelch(input, output, channel, std::get<3>(args->args),
                                std::get<4>(args->args));
}

// 2: Decipherment construction.
using TrainBaumWelchInnerArgs2 =
    std::tuple<const FstClass &, FarReaderClass *, MutableFstClass *,
               ExpectationTableType, const BaumWelchTrainOptions &>;

using TrainBaumWelchArgs2 =
    WithReturnValue<bool, TrainBaumWelchInnerArgs2>;

template <class Arc>
void TrainBaumWelch(TrainBaumWelchArgs2 *args) {
  const Fst<Arc> &input = *(std::get<0>(args->args).GetFst<Arc>());
  FarReader<Arc> *output = std::get<1>(args->args)->GetFarReader<Arc>();
  MutableFst<Arc> *channel = std::get<2>(args->args)->GetMutableFst<Arc>();
  args->retval = TrainBaumWelch(input, output, channel, std::get<3>(args->args),
                                std::get<4>(args->args));
}

bool TrainBaumWelch(
    FarReaderClass *input, FarReaderClass *output, MutableFstClass *channel,
    ExpectationTableType etype = STATE_ILABEL,
    const BaumWelchTrainOptions &opts = BaumWelchTrainOptions());

bool TrainBaumWelch(
    const FstClass &input, FarReaderClass *output, MutableFstClass *channel,
    ExpectationTableType etype = STATE_ILABEL,
    const BaumWelchTrainOptions &opts = BaumWelchTrainOptions());

}  // namespace script
}  // namespace fst

#endif  // BAUMWELCH_TRAINSCRIPT_H_

