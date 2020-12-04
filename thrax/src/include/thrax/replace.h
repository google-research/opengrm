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
// Interface to Fst Replace(). See under Execute() for implementation details.

#ifndef THRAX_REPLACE_H_
#define THRAX_REPLACE_H_

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include <fst/replace.h>
#include <fst/rmepsilon.h>
#include <thrax/datatype.h>
#include <thrax/function.h>

// TODO(rws): Figure out what to do with if we are keeping symbol tables around
// here. It looks as if Replace() may take care of this since it does the
// appropriate symbol table equivalence testing.

DECLARE_bool(save_symbols);  // From util/flags.cc.

namespace thrax {
namespace function {

template <typename Arc>
class Replace : public Function<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;
  using ReplaceTransducer = ::fst::ReplaceFst<Arc>;
  using ReplaceOptions = ::fst::ReplaceFstOptions<Arc>;
  using Label = typename Arc::Label;

  Replace() {}
  ~Replace() final {}

 protected:
  std::unique_ptr<DataType> Execute(
      const std::vector<std::unique_ptr<DataType>>& args) final {
    if (args.size() < 3) {
      std::cout << "Replace: Expected at least 3 arguments but got "
                << args.size() << std::endl;
      return nullptr;
    }
    // First transducer should be a single path transducer where each
    // consecutive nth input label is the replacement symbol for the nth
    // transducer. The root symbol must be the first in this label
    // transducer. We also check to see if the number of arcs extracted from
    // this transducer matches the number of remaining transducers.
    for (int i = 0; i < args.size(); ++i) {
      if (!args[i]->is<Transducer*>()) {
        std::cout << "Replace: all arguments must be FSTs: argument " << i
                  << " is not." << std::endl;
        return nullptr;
      }
    }
    MutableTransducer label_transducer(**args[0]->get<Transducer*>());
    std::vector<Label> labels;
    ExtractReplacementLabels(&label_transducer, &labels);
    if (labels.empty()) {
      std::cout << "Replace: No labels provided" << std::endl;
      return nullptr;
    }
    if (args.size() - 1 != labels.size()) {
      std::cout << "Replace: Number of replacement FSTs " << args.size() - 1
                << " does not match the number of replacement labels "
                << labels.size() << std::endl;
      return nullptr;
    }
    Label root = labels[0];
    std::vector<std::pair<Label, const Transducer*> > ifst_array;
    for (int i = 1; i < args.size(); ++i) {
      const auto* fst = *args[i]->get<Transducer*>();
      ifst_array.emplace_back(labels[i - 1], fst);
    }
    // Explicitly constructs ReplaceFst so we can check for cyclic dependencies
    // before attempting expansion.
    ::fst::ReplaceFstOptions<Arc> opts(root,
                                           ::fst::REPLACE_LABEL_NEITHER,
                                           ::fst::REPLACE_LABEL_NEITHER, 0);
    opts.gc = true;     // These options to the underlying cache supposedly
    opts.gc_limit = 0;  // result in faster batch expansion.
    ReplaceTransducer replace(ifst_array, opts);
    if (replace.CyclicDependencies()) {
      std::cout << "Replace: Cyclic dependencies detected; cannot expand";
      return nullptr;
    }
    auto output = std::make_unique<MutableTransducer>();
    *output = replace;  // Expansion.
    return std::make_unique<DataType>(std::move(output));
  }

 private:
  void ExtractReplacementLabels(MutableTransducer* fst,
                                std::vector<Label>* labels) {
    ::fst::RmEpsilon(fst);
    auto s = fst->Start();
    while (fst->Final(s) == Arc::Weight::Zero()) {
      if (fst->NumArcs(s) != 1) {
        std::cout << "Label transducer must have exactly one label arc "
                  << "from every state" << std::endl;
        labels->clear();
        return;
      }
      ::fst::ArcIterator<MutableTransducer> aiter(*fst, s);
      const auto& arc = aiter.Value();
      labels->push_back(arc.ilabel);
      s = arc.nextstate;
    }
  }

  Replace<Arc>(const Replace<Arc>&) = delete;
  Replace<Arc>& operator=(const Replace<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_REPLACE_H_
