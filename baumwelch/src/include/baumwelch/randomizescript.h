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

#ifndef BAUMWELCH_RANDOMIZESCRIPT_H_
#define BAUMWELCH_RANDOMIZESCRIPT_H_

#include <random>

#include <fst/script/fst-class.h>
#include <baumwelch/randomize.h>

namespace fst {
namespace script {

using RandomizeBaumWelchArgs = std::tuple<MutableFstClass *, uint64>;

template <class Arc>
void RandomizeBaumWelch(RandomizeBaumWelchArgs *args) {
  MutableFst<Arc> *model = std::get<0>(*args)->GetMutableFst<Arc>();
  RandomizeBaumWelch(model, std::get<1>(*args));
}

void RandomizeBaumWelch(MutableFstClass *fst,
                        uint64 seed = std::random_device()());

}  // namespace script
}  // namespace fst

#endif  // BAUMWELCH_RANDOMIZESCRIPT_H_

