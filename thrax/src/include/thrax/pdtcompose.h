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
// Composes two FSTs together, where one is a pushdown transducer
// (nlp/fst/extensions/pdt). The PDT may be either be the first or second
// transducer, as controlled by the fourth (string) argument to the
// function. The third argument is an FST that defines the alphabet of paired
// left and right parenthesis. For example, the following is a valid
// specification:
//
// 0  1  (  )
// 0  1  [  ]
// 0  1  <  >
// 1
//
// Parenthesis pairs can of course involve any labels one chooses, and the
// left/right pairs must be defined by the grammar writer. E.g.:
//
// parens = ("(" : ")") | ("[" : "]") | ("<" : ">");
//
// This function leaves its arguments unexpanded (if they weren't expanded to
// begin with) and creates an on-the-fly ComposeFst.
//
// If up to four arguments are provided, then we will arcsort neither of the
// input FSTs.  Alternatively, the fifth argument can be a string flag to
// control the sorting: - 'left' will arcsort the left FST on its output tape.
// - 'right' will arcsort the right FST on its input tape.  - 'both' will
// arcsort both the FSTs as above.

#ifndef THRAX_PDTCOMPOSE_H_
#define THRAX_PDTCOMPOSE_H_

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <fst/extensions/pdt/compose.h>
#include <fst/arcsort.h>
#include <thrax/datatype.h>
#include <thrax/function.h>
#include <thrax/make-parens-pair-vector.h>

DECLARE_bool(save_symbols);  // From util/flags.cc.

namespace thrax {
namespace function {

template <typename Arc>
class PdtCompose : public Function<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;
  using Label = typename Arc::Label;

  PdtCompose() {}
  ~PdtCompose() final {}

 protected:
  std::unique_ptr<DataType> Execute(
      const std::vector<std::unique_ptr<DataType>>& args) final {
    if (args.size() < 3 || args.size() > 5) {
      std::cout << "PdtCompose: Expected 3-5 arguments but got " << args.size()
                << std::endl;
      return nullptr;
    }
    if (!args[0]->is<Transducer*>()
        || !args[1]->is<Transducer*>()
        || !args[2]->is<Transducer*>()) {
      std::cout << "PdtCompose: First three arguments should be FSTs"
                << std::endl;
      return nullptr;
    }
    const auto* left = *args[0]->get<Transducer*>();
    const auto* right = *args[1]->get<Transducer*>();
    if (FLAGS_save_symbols) {
      if (!CompatSymbols(left->OutputSymbols(), right->InputSymbols())) {
        std::cout << "PdtCompose: output symbol table of 1st argument "
                  << "does not match input symbol table of 2nd argument"
                  << std::endl;
        return nullptr;
      }
    }
    MutableTransducer parens_transducer(**args[2]->get<Transducer*>());
    std::vector<std::pair<Label, Label>> parens;
    MakeParensPairVector(parens_transducer, &parens);
    bool left_pdt = false;
    if (args.size() > 3) {
      if (!args[3]->is<std::string>()) {
        std::cout << "PdtCompose: Expected string for argument 4" << std::endl;
        return nullptr;
      }
      const auto& pdt_direction = *args[3]->get<std::string>();
      if (pdt_direction != "left_pdt" && pdt_direction != "right_pdt") {
        std::cout
            << "PdtCompose: Expected 'left_pdt' or 'right_pdt' for argument 4"
            << std::endl;
        return nullptr;
      }
      if (pdt_direction == "left_pdt") left_pdt = true;
    }
    bool delete_left = false;
    bool delete_right = false;
    if (args.size() == 5) {
      if (!args[4]->is<std::string>()) {
        std::cout << "PdtCompose: Expected string for argument 5" << std::endl;
        return nullptr;
      }
      const auto& sort_mode = *args[4]->get<std::string>();
      if (sort_mode != "left" && sort_mode != "right" && sort_mode != "both") {
        std::cout
            << "PdtCompose: Expected 'left', 'right', or 'both' for argument 5"
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
    auto output = std::make_unique<MutableTransducer>();
    auto opts = ::fst::PdtComposeOptions();
    opts.connect = false;
    if (left_pdt) {
      ::fst::Compose(*left, parens, *right, output.get(), opts);
    } else {
      ::fst::Compose(*left, *right, parens, output.get(), opts);
    }
    if (delete_left) delete left;
    if (delete_right) delete right;
    return std::make_unique<DataType>(std::move(output));
  }

 private:
  PdtCompose<Arc>(const PdtCompose<Arc>&) = delete;
  PdtCompose<Arc>& operator=(const PdtCompose<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_PDTCOMPOSE_H_
