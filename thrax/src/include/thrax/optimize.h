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
// This function does a cleaning up of an FST by determinizing and minimizing
// it. If it's a transducer, we encode the arcs beforehand.

#ifndef THRAX_OPTIMIZE_H_
#define THRAX_OPTIMIZE_H_

#include <iostream>
#include <memory>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/vector-fst.h>
#include <thrax/algo/optimize.h>
#include <thrax/datatype.h>
#include <thrax/function.h>

namespace thrax {
namespace function {

template <typename Arc>
class Optimize : public UnaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  Optimize() {}
  ~Optimize() final {}

  // This function is public since other functions sometimes like to call on
  // this without having to go through the whole function registration
  // rigmarole.
  static std::unique_ptr<Transducer> ActuallyOptimize(
      const Transducer& fst, bool compute_props = false) {
    auto output = std::make_unique<MutableTransducer>(fst);
    ::fst::Optimize(output.get(), compute_props);
    return output;
  }

  static std::unique_ptr<Transducer> ActuallyOptimizeDifferenceRhs(
      const Transducer& fst, bool compute_props = false) {
    auto output = std::make_unique<MutableTransducer>(fst);
    ::fst::OptimizeDifferenceRhs(output.get(), compute_props);
    return output;
  }

 protected:
  std::unique_ptr<Transducer> UnaryFstExecute(
      const Transducer& fst,
      const std::vector<std::unique_ptr<DataType>>& args) final {
    if (args.size() != 1) {
      std::cout << "Optimize: Expected 1 argument but got " << args.size()
                << std::endl;
      return nullptr;
    }

    return ActuallyOptimize(fst);
  }

 private:
  Optimize<Arc>(const Optimize<Arc>&) = delete;
  Optimize<Arc>& operator=(const Optimize<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_OPTIMIZE_H_
