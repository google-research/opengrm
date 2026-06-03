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

#ifndef OPENGRM_BAUMWELCH_RANDOMIZESCRIPT_H_
#define OPENGRM_BAUMWELCH_RANDOMIZESCRIPT_H_

#include <cstdint>
#include <tuple>

#include "absl/random/bit_gen_ref.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/script/fst-class.h"
#include "opengrm/baumwelch/randomize.h"

namespace fst {
namespace script {

using BaumWelchRandomizeArgs = std::tuple<absl::BitGenRef, MutableFstClass*>;

template <class Arc>
void Randomize(BaumWelchRandomizeArgs* args) {
  MutableFst<Arc>* model = std::get<1>(*args)->GetMutableFst<Arc>();
  Randomize(std::get<0>(*args), model);
}

void Randomize(absl::BitGenRef bit_gen, MutableFstClass* fst);

}  // namespace script
}  // namespace fst

#endif  // OPENGRM_BAUMWELCH_RANDOMIZESCRIPT_H_
