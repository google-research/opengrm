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
// Determinize the single FST argument.

#ifndef THRAX_DETERMINIZE_H_
#define THRAX_DETERMINIZE_H_

#include <iostream>
#include <memory>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/determinize.h>
#include <thrax/datatype.h>
#include <thrax/function.h>

namespace thrax {
namespace function {

template <typename Arc>
class Determinize : public UnaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  Determinize() {}
  ~Determinize() final {}

 protected:
  std::unique_ptr<Transducer> UnaryFstExecute(
      const Transducer& fst,
      const std::vector<std::unique_ptr<DataType>>& args) final {
    if (args.size() != 1) {
      std::cout << "Determinize: Expected 1 argument but got " << args.size()
                << std::endl;
      return nullptr;
    }
    auto output = std::make_unique<MutableTransducer>();
    ::fst::Determinize(fst, output.get());
    return output;
  }

 private:
  Determinize<Arc>(const Determinize<Arc>&) = delete;
  Determinize<Arc>& operator=(const Determinize<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_DETERMINIZE_H_
