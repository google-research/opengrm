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
// Arc-sorts the single FST argument.

#ifndef THRAX_ARCSORT_H_
#define THRAX_ARCSORT_H_

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/arcsort.h>
#include <thrax/datatype.h>
#include <thrax/function.h>

namespace thrax {
namespace function {

template <typename Arc>
class ArcSort : public UnaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  ArcSort() {}
  ~ArcSort() final{};

 protected:
  std::unique_ptr<Transducer> UnaryFstExecute(
      const Transducer& fst,
      const std::vector<std::unique_ptr<DataType>>& args) final {
    if (args.size() != 2) {
      std::cout << "ArcSort: Expected 2 arguments but received " << args.size()
                << std::endl;
      return nullptr;
    }
    if (!args[1]->is<std::string>()) {
      std::cout << "ArcSort: Expected string for argument 2" << std::endl;
      return nullptr;
    }
    const auto& sort = *args[1]->get<std::string>();
    if (sort == "input") {
      static const ::fst::ILabelCompare<Arc> icomp;
      return std::make_unique<
          ::fst::ArcSortFst<Arc, ::fst::ILabelCompare<Arc>>>(fst,
                                                                     icomp);
    } else if (sort == "output") {
      static const ::fst::OLabelCompare<Arc> ocomp;
      return std::make_unique<
          ::fst::ArcSortFst<Arc, fst::OLabelCompare<Arc>>>(fst, ocomp);
    } else {
      std::cout << "ArcSort: Invalid sort parameter: " << sort
                << " (should be 'input' or 'output')" << std::endl;
      return nullptr;
    }
  }

 private:
  ArcSort<Arc>(const ArcSort<Arc>&) = delete;
  ArcSort<Arc>& operator=(const ArcSort<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_ARCSORT_H_
