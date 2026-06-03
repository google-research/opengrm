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

// This function removes epsilon arcs from an FST. The argument FST is be
// expanded so that epsilon arcs can be removed in place.

#ifndef OPENGRM_THRAX_WALKER_UTIL_FUNCTION_RMEPSILON_H_
#define OPENGRM_THRAX_WALKER_UTIL_FUNCTION_RMEPSILON_H_

#include <iostream>
#include <memory>
#include <ostream>
#include <vector>

#include "openfst/lib/fst.h"
#include "openfst/lib/rmepsilon.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"
#include "opengrm/thrax/walker/util/function/function.h"

namespace thrax {
namespace function {

template <typename Arc>
class RmEpsilon : public UnaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  RmEpsilon() = default;
  ~RmEpsilon() final = default;

 protected:
  std::unique_ptr<Transducer> UnaryFstExecute(
      const Transducer& fst,
      const std::vector<std::unique_ptr<DataType>>& args) const final {
    if (args.size() != 1) {
      std::cout << "RmEpsilon: Expected 1 argument but got " << args.size()
                << std::endl;
      return nullptr;
    }
    auto output = std::make_unique<MutableTransducer>(fst);
    ::fst::RmEpsilon(output.get());
    return output;
  }

 private:
  RmEpsilon(const RmEpsilon<Arc>&) = delete;
  RmEpsilon<Arc>& operator=(const RmEpsilon<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // OPENGRM_THRAX_WALKER_UTIL_FUNCTION_RMEPSILON_H_
