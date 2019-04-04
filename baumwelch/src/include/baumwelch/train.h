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

#ifndef BAUMWELCH_TRAIN_H_
#define BAUMWELCH_TRAIN_H_

#include <baumwelch/baumwelch.h>

// Helper functions for users who don't require the low-level control provided
// by the BaumWelch class.

namespace fst {

// This instantiates the various ExpectationTable templates by switching on
// the ExpectationTableType enum.
template <class Arc, class Data>
bool TrainBaumWelch(
    Data *data,
    MutableFst<Arc> *channel,
    ExpectationTableType etype = STATE_ILABEL,
    const BaumWelchTrainOptions &opts = BaumWelchTrainOptions()) {
  switch (etype) {
    case GLOBAL: {
      BaumWelch<Arc, Data, GlobalExpectationTable<Arc>> baumwelch(*channel);
      const auto converged = baumwelch.Train(data, opts);
      *channel = baumwelch.Channel();
      return converged;
    }
    case STATE: {
      BaumWelch<Arc, Data, StateExpectationTable<Arc>> baumwelch(*channel);
      const auto converged = baumwelch.Train(data, opts);
      *channel = baumwelch.Channel();
      return converged;
    }
    case ILABEL: {
      BaumWelch<Arc, Data, ILabelExpectationTable<Arc>> baumwelch(*channel);
      const auto converged = baumwelch.Train(data, opts);
      *channel = baumwelch.Channel();
      return converged;
    }
    case STATE_ILABEL: {
      BaumWelch<Arc, Data, StateILabelExpectationTable<Arc>> baumwelch(
          *channel);
      const auto converged = baumwelch.Train(data, opts);
      *channel = baumwelch.Channel();
      return converged;
    }
  }
  // Unreachable.
  return false;
}

// This instantiates the paired construction.
template <class Arc>
bool TrainBaumWelch(
    FarReader<Arc> *input, FarReader<Arc> *output, MutableFst<Arc> *channel,
    ExpectationTableType etype = STATE_ILABEL,
    const BaumWelchTrainOptions &opts = BaumWelchTrainOptions()) {
  internal::PairedData<Arc> data(input, output);
  return TrainBaumWelch(&data, channel, etype, opts);
}

// This instantiates the decipherment construction.
template <class Arc>
bool TrainBaumWelch(
    const Fst<Arc> &input, FarReader<Arc> *output, MutableFst<Arc> *channel,
    ExpectationTableType etype = STATE_ILABEL,
    const BaumWelchTrainOptions &opts = BaumWelchTrainOptions()) {
  internal::DeciphermentData<Arc> data(input, output);
  return TrainBaumWelch(&data, channel, etype, opts);
}

}  // namespace fst

#endif  // BAUMWELCH_TRAIN_H_

