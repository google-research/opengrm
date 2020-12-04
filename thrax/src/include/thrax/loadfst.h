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
// Loads an FST from the provided filename.

#ifndef THRAX_LOADFST_H_
#define THRAX_LOADFST_H_

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/compat/utils.h>
#include <fst/fst.h>
#include <thrax/datatype.h>
#include <thrax/function.h>

DECLARE_bool(save_symbols);  // From util/flags.cc.
DECLARE_string(indir);  // From util/flags.cc.

namespace thrax {
namespace function {

template <typename Arc>
class LoadFst : public Function<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;

  LoadFst() {}
  ~LoadFst() final {}

 protected:
  std::unique_ptr<DataType> Execute(
      const std::vector<std::unique_ptr<DataType>>& args) final {
    if (args.size() != 1) {
      std::cout << "LoadFst: Expected 1 argument but got " << args.size()
                << std::endl;
      return nullptr;
    }
    if (!args[0]->is<std::string>()) {
      std::cout << "LoadFst: Expected string (path) for argument 1"
                << std::endl;
      return nullptr;
    }
    const auto& file =
        JoinPath(FLAGS_indir, *args[0]->get<std::string>());
    VLOG(2) << "Loading FST: " << file;
    auto fst = fst::WrapUnique(Transducer::Read(file));
    if (!fst) {
      std::cout << "LoadFst: Failed to load FST from file: " << file
                << std::endl;
      return nullptr;
    }
    if (FLAGS_save_symbols) {
      if (!fst->InputSymbols()) {
        LOG(WARNING) << "LoadFst: FLAGS_save_symbols is set "
                     << "but fst has no input symbols";
      }
      if (!fst->OutputSymbols()) {
        LOG(WARNING) << "LoadFst: FLAGS_save_symbols is set "
                     << "but fst has no output symbols";
      }
    }
    return std::make_unique<DataType>(std::move(fst));
  }

 private:
  LoadFst<Arc>(const LoadFst<Arc>&) = delete;
  LoadFst<Arc>& operator=(const LoadFst<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_LOADFST_H_
