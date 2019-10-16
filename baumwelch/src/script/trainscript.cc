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

#include <baumwelch/trainscript.h>

#include <fst/script/script-impl.h>
#include <baumwelch/getters.h>

namespace fst {
namespace script {

bool TrainBaumWelch(FarReaderClass *input, FarReaderClass *output,
                    MutableFstClass *channel, ExpectationTableType etype,
                    const BaumWelchTrainOptions &opts) {
  if (!internal::ArcTypesMatch(*input, *channel, "TrainBaumWelch") ||
      !internal::ArcTypesMatch(*output, *channel, "TrainBaumWelch")) {
    channel->SetProperties(kError, kError);
    return false;
  }
  TrainBaumWelchInnerArgs1 iargs(input, output, channel, etype, opts);
  TrainBaumWelchArgs1 args(iargs);
  Apply<Operation<TrainBaumWelchArgs1>>("TrainBaumWelch", channel->ArcType(),
                                        &args);
  return args.retval;
}

// 2: Decipherment construction.
bool TrainBaumWelch(const FstClass &input, FarReaderClass *output,
                    MutableFstClass *channel, ExpectationTableType etype,
                    const BaumWelchTrainOptions &opts) {
  if (!internal::ArcTypesMatch(input, *channel, "TrainBaumWelch") ||
      !internal::ArcTypesMatch(*output, *channel, "TrainBaumWelch")) {
    channel->SetProperties(kError, kError);
    return false;
  }
  TrainBaumWelchInnerArgs2 iargs(input, output, channel, etype, opts);
  TrainBaumWelchArgs2 args(iargs);
  Apply<Operation<TrainBaumWelchArgs2>>("TrainBaumWelch", channel->ArcType(),
                                        &args);
  return args.retval;
}

REGISTER_FST_OPERATION_3ARCS(TrainBaumWelch, TrainBaumWelchArgs1);
REGISTER_FST_OPERATION_3ARCS(TrainBaumWelch, TrainBaumWelchArgs2);

}  // namespace script
}  // namespace fst

