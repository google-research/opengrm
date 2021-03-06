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
// LenientlyComposes two FSTs together.
//
// Requires three arguments, namely two transducers to be leniently composed,
// and a third FST representing the transitive closure of the universal alphabet
// (sigma star).

#ifndef THRAX_LENIENTLYCOMPOSE_H_
#define THRAX_LENIENTLYCOMPOSE_H_

#include <iostream>
#include <memory>
#include <vector>

#include <fst/vector-fst.h>
#include <thrax/algo/lenientlycompose.h>
#include <thrax/datatype.h>
#include <thrax/function.h>

DECLARE_bool(save_symbols);  // From util/flags.cc.

namespace thrax {
namespace function {

template <typename Arc>
class LenientlyCompose : public Function<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  LenientlyCompose() {}
  ~LenientlyCompose() final {}

 protected:
  std::unique_ptr<DataType> Execute(
      const std::vector<std::unique_ptr<DataType>>& args) final {
    if (args.size() != 3) {
      std::cout << "LenientyCompose: Expected 3 arguments but got "
                << args.size()
                << std::endl;
      return nullptr;
    }
    if (!args[0]->is<Transducer*>() ||
        !args[1]->is<Transducer*>() ||
        !args[2]->is<Transducer*>()) {
      std::cout << "LenientlyCompose: Arguments should be FSTs" << std::endl;
      return nullptr;
    }
    const auto* left = *args[0]->get<Transducer*>();
    const auto* right = *args[1]->get<Transducer*>();
    const auto* sigstar = *args[2]->get<Transducer*>();
    if (FST_FLAGS_save_symbols) {
      if (!CompatSymbols(left->OutputSymbols(), right->InputSymbols())) {
        std::cout << "LenientlyCompose: output symbol table of 1st argument "
                  << "does not match input symbol table of 2nd argument"
                  << std::endl;
        return nullptr;
      }
      // LenientlyCompose computes the difference between sigstar and the input
      // projection of the first FST, so that means the same comparison as is
      // done in Difference must be done here.
      if (!::fst::CompatSymbols(sigstar->InputSymbols(),
                                    left->InputSymbols())) {
        std::cout << "LenientlyCompose: Input symbol of 1st argument "
                  << "does not match input symbol table of sigma star argument"
                  << std::endl;
        return nullptr;
      }
      if (!::fst::CompatSymbols(sigstar->OutputSymbols(),
                                    left->InputSymbols() /* sic */)) {
        std::cout << "LenientlyCompose: Input symbol of 1st argument "
                  << "does not match output symbol table of sigma star argument"
                  << std::endl;
        return nullptr;
      }
    }
    auto output = std::make_unique<MutableTransducer>();
    ::fst::LenientlyCompose(*left, *right, *sigstar, output.get());
    return std::make_unique<DataType>(std::move(output));
  }

 private:
  LenientlyCompose<Arc>(const LenientlyCompose<Arc>&) = delete;
  LenientlyCompose<Arc>& operator=(const LenientlyCompose<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_LENIENTLYCOMPOSE_H_
