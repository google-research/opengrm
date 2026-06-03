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

// Wrapper for the rewrite function, where we have two FSTs and rewrite the
// the first into the second.  This function creates only on-the-fly
// transducers.

#ifndef OPENGRM_THRAX_WALKER_UTIL_FUNCTION_REWRITE_H_
#define OPENGRM_THRAX_WALKER_UTIL_FUNCTION_REWRITE_H_

#include <iostream>
#include <memory>
#include <ostream>
#include <vector>

#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/operators/cross.h"
#include "opengrm/thrax/walker/util/datatype.h"
#include "opengrm/thrax/walker/util/function/function.h"

ABSL_DECLARE_FLAG(bool, save_symbols);  // From util/flags.cc.

namespace thrax {
namespace function {

template <typename Arc>
class Rewrite : public BinaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  Rewrite() = default;
  ~Rewrite() final = default;

 protected:
  std::unique_ptr<Transducer> BinaryFstExecute(
      const Transducer& left, const Transducer& right,
      const std::vector<std::unique_ptr<DataType>>& args) const final {
    if (args.size() != 2) {
      std::cout << "Rewrite: Expected 2 arguments but got " << args.size()
                << std::endl;
      return nullptr;
    }
    // If we keep the symbol tables and if either the input or the output is not
    // an acceptor, then the output symbols of the left and the output symbols
    // of the right must match.
    if (absl::GetFlag(FLAGS_save_symbols) &&
        (!left.Properties(::fst::kAcceptor, true) ||
         !right.Properties(::fst::kAcceptor, true))) {
      if (!CompatSymbols(left.OutputSymbols(), right.InputSymbols())) {
        std::cout << "Rewrite: output symbol table of 1st argument "
                  << "does not match input symbol table of 2nd argument "
                  << "and at least one is not an acceptor" << std::endl;
        return nullptr;
      }
    }
    auto output = std::make_unique<MutableTransducer>();
    ::fst::Cross(left, right, output.get());
    return output;
  }

 private:
  Rewrite(const Rewrite<Arc>&) = delete;
  Rewrite<Arc>& operator=(const Rewrite<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // OPENGRM_THRAX_WALKER_UTIL_FUNCTION_REWRITE_H_
