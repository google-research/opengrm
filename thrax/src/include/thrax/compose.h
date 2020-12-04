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
// Composes two FSTs together. This function leaves its arguments unexpanded
// (if they weren't expanded to begin with) and creates an on-the-fly
// ComposeFst.
//
// If only two arguments are provided, then we will arcsort neither of the input
// FSTs. Alternatively, the third argument can be a string flag to control the
// sorting:
//   - 'left' will arcsort the left FST on its output tape.
//   - 'right' will arcsort the right FST on its input tape.
//   - 'both' will arcsort both the FSTs as above.

#ifndef THRAX_COMPOSE_H_
#define THRAX_COMPOSE_H_

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/compose.h>
#include <thrax/datatype.h>
#include <thrax/function.h>

DECLARE_bool(save_symbols);  // From util/flags.cc.

namespace thrax {
namespace function {

template <typename Arc>
class Compose : public Function<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;

  Compose() {}
  ~Compose() final {}

 protected:
  std::unique_ptr<DataType> Execute(
      const std::vector<std::unique_ptr<DataType>>& args) final {
    if (args.size() != 2 && args.size() != 3) {
      std::cout << "Compose: Expected 2 or 3 arguments but got " << args.size()
                << std::endl;
      return nullptr;
    }
    if (!args[0]->is<Transducer*>() || !args[1]->is<Transducer*>()) {
      std::cout << "Compose: First two arguments should be FSTs" << std::endl;
      return nullptr;
    }
    const auto* left = *args[0]->get<Transducer*>();
    const auto* right = *args[1]->get<Transducer*>();
    bool delete_left = false;
    bool delete_right = false;
    if (FLAGS_save_symbols) {
      if (!CompatSymbols(left->OutputSymbols(), right->InputSymbols())) {
        std::cout << "Compose: output symbol table of 1st argument "
                  << "does not match input symbol table of 2nd argument"
                  << std::endl;
        return nullptr;
      }
    }
    if (args.size() == 3) {
      if (!args[2]->is<std::string>()) {
        std::cout << "Compose: Expected string for argument 3" << std::endl;
        return nullptr;
      }
      const auto& sort_mode = *args[2]->get<std::string>();
      if (sort_mode != "left" && sort_mode != "right" && sort_mode != "both") {
        std::cout
            << "Compose: Expected 'left', 'right', or 'both' for argument 3"
            << std::endl;
        return nullptr;
      }
      if (sort_mode != "right") {
        static const ::fst::OLabelCompare<Arc> ocomp;
        left = new ::fst::ArcSortFst<Arc, ::fst::OLabelCompare<Arc>>(
            *left, ocomp);
        delete_left = true;
      }
      if (sort_mode != "left") {
        static const ::fst::ILabelCompare<Arc> icomp;
        right = new ::fst::ArcSortFst<Arc, ::fst::ILabelCompare<Arc>>(
            *right, icomp);
        delete_right = true;
      }
    }
    auto output = std::make_unique<::fst::ComposeFst<Arc>>(*left, *right);
    if (delete_left) delete left;
    if (delete_right) delete right;
    return std::make_unique<DataType>(std::move(output));
  }

 private:
  Compose<Arc>(const Compose<Arc>&) = delete;
  Compose<Arc>& operator=(const Compose<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_COMPOSE_H_
