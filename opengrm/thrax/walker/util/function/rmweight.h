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

// Removes weights from arcs and final states (i.e., by setting them all to
// Weight::One()).

#ifndef OPENGRM_THRAX_WALKER_UTIL_FUNCTION_RMWEIGHT_H_
#define OPENGRM_THRAX_WALKER_UTIL_FUNCTION_RMWEIGHT_H_

#include <iostream>
#include <memory>
#include <ostream>
#include <vector>

#include "openfst/lib/arc-map.h"
#include "openfst/lib/fst.h"
#include "opengrm/thrax/walker/util/datatype.h"
#include "opengrm/thrax/walker/util/function/function.h"

namespace thrax {
namespace function {

template <typename Arc>
class RmWeight : public UnaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using RmWeightMapper = ::fst::RmWeightMapper<Arc>;
  using ToArc = typename RmWeightMapper::ToArc;
  using RmWeightFst = ::fst::ArcMapFst<Arc, ToArc, RmWeightMapper>;

  RmWeight() = default;
  ~RmWeight() final = default;

 protected:
  std::unique_ptr<Transducer> UnaryFstExecute(
      const Transducer& fst,
      const std::vector<std::unique_ptr<DataType>>& args) const final {
    if (args.size() != 1) {
      std::cout << "RmWeight: Expected 1 argument but got " << args.size()
                << std::endl;
      return nullptr;
    }
    return std::make_unique<RmWeightFst>(fst);
  }

 private:
  RmWeight(const RmWeight<Arc>&) = delete;
  RmWeight<Arc>& operator=(const RmWeight<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // OPENGRM_THRAX_WALKER_UTIL_FUNCTION_RMWEIGHT_H_
