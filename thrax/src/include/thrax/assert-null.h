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
// Assert the first argument is null.
//
// Thus for example if I have a transducer "trans", I might test if applying
// "trans" to "foo" is null:
//
// equality = AssertNull[foo @ trans];

#ifndef THRAX_ASSERT_NULL_H_
#define THRAX_ASSERT_NULL_H_

#include <iostream>
#include <memory>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/project.h>
#include <fst/rmepsilon.h>
#include <thrax/datatype.h>
#include <thrax/function.h>

DECLARE_bool(save_symbols);  // From util/flags.cc.

namespace thrax {
namespace function {

// TODO(rws): Some day we should make this so that it doesn't return a
// value, but merely runs the assertion. That, however, would require changing
// the parser.
template <typename Arc>
class AssertNull : public UnaryFstFunction<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  AssertNull() {}
  ~AssertNull() final {}

 protected:
  std::unique_ptr<Transducer> UnaryFstExecute(
      const Transducer& left,
      const std::vector<std::unique_ptr<DataType>>& args) final {
    if (args.size() != 1) {
      std::cout << "AssertNull: Expected 1 argument but got "
                << args.size() << std::endl;
      return nullptr;
    }
    auto mutable_left = std::make_unique<MutableTransducer>(left);
    ::fst::Project(mutable_left.get(), ::fst::ProjectType::OUTPUT);
    ::fst::RmEpsilon(mutable_left.get());
    if (mutable_left->NumStates() != 0) {
      std::cout << "Argument to AssertNull is not null:"
                << std::endl;
      return nullptr;
    }
    return mutable_left;
  }

 private:
  AssertNull<Arc>(const AssertNull<Arc>&) = delete;
  AssertNull<Arc>& operator=(const AssertNull<Arc>&) = delete;
};


}  // namespace function
}  // namespace thrax

#endif  // THRAX_ASSERT_NULL_H_
