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

#ifndef OPENGRM_OPERATORS_OPTIMIZESCRIPT_H_
#define OPENGRM_OPERATORS_OPTIMIZESCRIPT_H_

#include <tuple>
#include <utility>

#include "openfst/lib/mutable-fst.h"
#include "openfst/script/fst-class.h"
#include "opengrm/operators/optimize.h"

namespace fst {
namespace script {

using FstOptimizeArgs = std::tuple<MutableFstClass*, bool>;

template <class Arc>
void Optimize(FstOptimizeArgs* args) {
  MutableFst<Arc>* fst = std::get<0>(*args)->GetMutableFst<Arc>();
  Optimize(fst, std::get<1>(*args));
}

void Optimize(MutableFstClass* fst, bool compute_props = false);

template <class Arc>
void OptimizeDifferenceRhs(FstOptimizeArgs* args) {
  MutableFst<Arc>* fst = std::get<0>(*args)->GetMutableFst<Arc>();
  OptimizeDifferenceRhs(fst, std::get<1>(*args));
}

void OptimizeDifferenceRhs(MutableFstClass* fst, bool compute_props = false);

}  // namespace script
}  // namespace fst

#endif  // OPENGRM_OPERATORS_OPTIMIZESCRIPT_H_
