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

// Wrapper for the concatenation function, which expands the second argument and
// concatenates into there (destructive-mode) or just uses ConcatFst
// (delayed-mode).

#ifndef OPENGRM_THRAX_WALKER_UTIL_FUNCTION_CONCAT_H_
#define OPENGRM_THRAX_WALKER_UTIL_FUNCTION_CONCAT_H_

#include <iostream>
#include <memory>
#include <ostream>
#include <vector>

#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "openfst/lib/concat.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"
#include "opengrm/thrax/walker/util/function/function.h"

ABSL_DECLARE_FLAG(bool, save_symbols);  // From util/flags.cc.

namespace thrax {
namespace function {

template <typename Arc>
class Concat : public BinaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  Concat() = default;
  ~Concat() final = default;

 protected:
  std::unique_ptr<Transducer> BinaryFstExecute(
      const Transducer& left, const Transducer& right,
      const std::vector<std::unique_ptr<DataType>>& args) const final {
    if (args.size() != 2) {
      std::cout << "Concat: Expected 2 arguments but got " << args.size()
                << std::endl;
      return nullptr;
    }
    if (absl::GetFlag(FLAGS_save_symbols)) {
      if (!::fst::CompatSymbols(left.InputSymbols(), right.InputSymbols())) {
        std::cout << "Concat: input symbol table of 1st argument "
                  << "does not match input symbol table of 2nd argument"
                  << std::endl;
        return nullptr;
      }
      if (!::fst::CompatSymbols(left.OutputSymbols(), right.OutputSymbols())) {
        std::cout << "Concat: output symbol table of 1st argument "
                  << "does not match output symbol table of 2nd argument"
                  << std::endl;
        return nullptr;
      }
    }
    auto mutable_right = std::make_unique<MutableTransducer>(right);
    ::fst::Concat(left, mutable_right.get());
    return mutable_right;
  }

 private:
  Concat(const Concat<Arc>&) = delete;
  Concat<Arc>& operator=(const Concat<Arc>&) = delete;
};

template <typename Arc>
class ConcatDelayed : public BinaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;

  ConcatDelayed() = default;
  ~ConcatDelayed() override = default;

 protected:
  std::unique_ptr<Transducer> BinaryFstExecute(
      const Transducer& left, const Transducer& right,
      const std::vector<std::unique_ptr<DataType>>& args) const final {
    if (args.size() != 2) {
      std::cout << "ConcatDelayed: Expected 2 arguments but got " << args.size()
                << std::endl;
      return nullptr;
    }
    if (absl::GetFlag(FLAGS_save_symbols)) {
      if (!::fst::CompatSymbols(left.InputSymbols(), right.InputSymbols())) {
        std::cout << "ConcatDelayed: input symbol table of 1st argument "
                  << "does not match input symbol table of 2nd argument"
                  << std::endl;
        return nullptr;
      }
      if (!::fst::CompatSymbols(left.OutputSymbols(), right.OutputSymbols())) {
        std::cout << "ConcatDelayed: output symbol table of 1st argument "
                  << "does not match output symbol table of 2nd argument"
                  << std::endl;
        return nullptr;
      }
    }
    return std::make_unique<::fst::ConcatFst<Arc>>(left, right);
  }

 private:
  ConcatDelayed(const ConcatDelayed<Arc>&) = delete;
  ConcatDelayed<Arc>& operator=(const ConcatDelayed<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // OPENGRM_THRAX_WALKER_UTIL_FUNCTION_CONCAT_H_
