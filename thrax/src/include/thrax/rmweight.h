// Copyright 2005-2020 Google LLC
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
//
// Removes weights from arcs and final states (i.e., by setting them all to
// Weight::One()).

#ifndef THRAX_RMWEIGHT_H_
#define THRAX_RMWEIGHT_H_

#include <memory>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/arc-map.h>
#include <thrax/datatype.h>
#include <thrax/function.h>

namespace thrax {
namespace function {

template <typename Arc>
class RmWeight : public UnaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;

  RmWeight() {}
  ~RmWeight() final {}

 protected:
  std::unique_ptr<Transducer> UnaryFstExecute(
      const Transducer& fst,
      const std::vector<std::unique_ptr<DataType>>& args) final {
    if (args.size() != 1) {
      std::cout << "RmWeight: Expected 1 argument but got " << args.size()
                << std::endl;
      return nullptr;
    }
    return fst::WrapUnique(
        MakeArcMapFst(fst, ::fst::RmWeightMapper<Arc>()).Copy());
  }

 private:
  RmWeight<Arc>(const RmWeight<Arc>&) = delete;
  RmWeight<Arc>& operator=(const RmWeight<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_RMWEIGHT_H_
