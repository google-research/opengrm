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
// This function inverts an FST using a delayed FST.

#ifndef THRAX_INVERT_H_
#define THRAX_INVERT_H_

#include <iostream>
#include <memory>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/invert.h>
#include <thrax/datatype.h>
#include <thrax/function.h>

namespace thrax {
namespace function {

template <typename Arc>
class Invert : public UnaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;

  Invert() {}
  ~Invert() final {}

 protected:
  std::unique_ptr<Transducer> UnaryFstExecute(
      const Transducer& fst,
      const std::vector<std::unique_ptr<DataType>>& args) final {
    if (args.size() != 1) {
      std::cout << "Invert: Expected 1 argument but got " << args.size()
                << std::endl;
      return nullptr;
    }
    return std::make_unique<::fst::InvertFst<Arc>>(fst);
  }

 private:
  Invert<Arc>(const Invert<Arc>&) = delete;
  Invert<Arc>& operator=(const Invert<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_INVERT_H_
