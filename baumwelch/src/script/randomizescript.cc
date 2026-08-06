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

#include <baumwelch/randomizescript.h>

#include <cstdint>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void Randomize(MutableFstClass *fst, uint64_t seed) {
  BaumWelchRandomizeArgs args{fst, seed};
  Apply<Operation<BaumWelchRandomizeArgs>>("Randomize", fst->ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(Randomize, BaumWelchRandomizeArgs);

}  // namespace script
}  // namespace fst

