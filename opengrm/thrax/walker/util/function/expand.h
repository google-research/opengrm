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

// This function takes an FST and expands it to a VectorFst. This function is
// cheap if the input is already an expanded VectorFst.

#ifndef OPENGRM_THRAX_WALKER_UTIL_FUNCTION_EXPAND_H_
#define OPENGRM_THRAX_WALKER_UTIL_FUNCTION_EXPAND_H_

#include <iostream>
#include <memory>
#include <ostream>
#include <vector>

#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"
#include "opengrm/thrax/walker/util/function/function.h"

namespace thrax {
namespace function {

template <typename Arc>
class Expand : public UnaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  Expand() = default;
  ~Expand() final = default;

 protected:
  std::unique_ptr<Transducer> UnaryFstExecute(
      const Transducer& fst,
      const std::vector<std::unique_ptr<DataType>>& args) const final {
    if (args.size() != 1) {
      std::cout << "Expand: Expected 1 argument but got " << args.size()
                << std::endl;
      return nullptr;
    }
    return std::make_unique<MutableTransducer>(fst);
  }

 private:
  Expand(const Expand<Arc>&) = delete;
  Expand<Arc>& operator=(const Expand<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // OPENGRM_THRAX_WALKER_UTIL_FUNCTION_EXPAND_H_
