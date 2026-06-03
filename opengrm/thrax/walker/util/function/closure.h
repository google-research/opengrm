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

// Provides (Kleene) closure operations for the provided FST. This expands the
// provided FST and then modifies it in place.

#ifndef OPENGRM_THRAX_WALKER_UTIL_FUNCTION_CLOSURE_H_
#define OPENGRM_THRAX_WALKER_UTIL_FUNCTION_CLOSURE_H_

#include <iostream>
#include <memory>
#include <ostream>
#include <vector>

#include "openfst/lib/closure.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/rational.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/operators/concatrange.h"
#include "opengrm/thrax/ast/fst-node.h"
#include "opengrm/thrax/walker/util/datatype.h"
#include "opengrm/thrax/walker/util/function/function.h"

namespace thrax {
namespace function {

template <typename Arc>
class Closure : public UnaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  Closure() = default;
  ~Closure() final = default;

 protected:
  std::unique_ptr<Transducer> UnaryFstExecute(
      const Transducer& fst,
      const std::vector<std::unique_ptr<DataType>>& args) const final {
    if (args.size() < 2) {
      std::cout << "Closure: Expected 2 or 4 arguments" << std::endl;
      return nullptr;
    }
    if (!args[1]->is<int>()) {
      std::cout << "Closure: Expected int/enum for argument 2" << std::endl;
      return nullptr;
    }
    const auto type = static_cast<RepetitionFstNode::RepetitionFstNodeType>(
        *args[1]->get<int>());
    auto output = std::make_unique<MutableTransducer>(fst);
    switch (type) {
      case RepetitionFstNode::STAR: {
        if (args.size() != 2) {
          std::cout << "Closure: Expected 2 arguments for star closure"
                    << std::endl;
          return nullptr;
        }
        ::fst::Closure(output.get(), fst::CLOSURE_STAR);
        break;
      }
      case RepetitionFstNode::PLUS: {
        if (args.size() != 2) {
          std::cout << "Closure: Expected 2 arguments for plus closure"
                    << std::endl;
          return nullptr;
        }
        ::fst::Closure(output.get(), fst::CLOSURE_PLUS);
        break;
      }
      case RepetitionFstNode::QUESTION: {
        if (args.size() != 2) {
          std::cout << "Closure: Expected 2 arguments for optional closure"
                    << std::endl;
          return nullptr;
        }
        ::fst::ConcatRange(output.get(), 0, 1);
        break;
      }
      case RepetitionFstNode::RANGE: {
        if (args.size() != 4) {
          std::cout << "Closure: Expected 4 arguments for range closure"
                    << std::endl;
          return nullptr;
        }
        for (int i = 2; i < 4; ++i) {
          if (!args[i]->is<int>()) {
            std::cout << "Closure: Expected int for argument " << i + 1
                      << " for "
                      << "range closure" << std::endl;
            return nullptr;
          }
        }
        const auto min = *args[2]->get<int>();
        const auto max = *args[3]->get<int>();
        ::fst::ConcatRange(output.get(), min, max);
        break;
      }
      default: {
        std::cout << "Closure: No implementation for RepetitionFstNode type "
                  << type;
        return nullptr;
      }
    }
    return output;
  }

 private:
  Closure(const Closure<Arc>&) = delete;
  Closure<Arc>& operator=(const Closure<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // OPENGRM_THRAX_WALKER_UTIL_FUNCTION_CLOSURE_H_
