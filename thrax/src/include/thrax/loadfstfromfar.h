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
// Loads an FST from a single FAR archive.

#ifndef THRAX_LOADFSTFROMFAR_H_
#define THRAX_LOADFSTFROMFAR_H_

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/compat/utils.h>
#include <fst/extensions/far/far.h>
#include <fst/vector-fst.h>
#include <thrax/datatype.h>
#include <thrax/function.h>

DECLARE_bool(save_symbols);  // From util/flags.cc.
DECLARE_string(indir);  // From util/flags.cc.

namespace thrax {
namespace function {

template <typename Arc>
class LoadFstFromFar : public Function<Arc> {
 public:
  using MutableTransducer = ::fst::VectorFst<Arc>;

  LoadFstFromFar() {}
  ~LoadFstFromFar() final {}

 protected:
  std::unique_ptr<DataType> Execute(
      const std::vector<std::unique_ptr<DataType>>& args) final {
    if (args.size() != 2) {
      std::cout << "LoadFstFromFar: Expected 2 arguments but got "
                << args.size() << std::endl;
      return nullptr;
    }
    if (!args[0]->is<std::string>()) {
      std::cout << "LoadFstFromFar: Expected string (path) for argument 1"
                << std::endl;
      return nullptr;
    }
    const auto& far_file =
        JoinPath(FLAGS_indir, *args[0]->get<std::string>());
    if (!args[1]->is<std::string>()) {
      std::cout << "LoadFstFromFar: Expected string (FST name) for argument 2"
                << std::endl;
      return nullptr;
    }
    const auto& fst_name = *args[1]->get<std::string>();
    VLOG(2) << "Loading FST " << fst_name << " from " << far_file;
    std::unique_ptr<::fst::FarReader<Arc>> reader(
        ::fst::FarReader<Arc>::Open(far_file));
    if (!reader) {
      std::cout << "LoadFstFromFar: Unable to open FAR: " << far_file
                << std::endl;
      return nullptr;
    }

    if (!reader->Find(fst_name)) {
      std::cout << "LoadFstFromFar: Unable to find FST: " << fst_name
                << std::endl;
      return nullptr;
    }
    auto fst = std::make_unique<MutableTransducer>(*(reader->GetFst()));
    if (FLAGS_save_symbols) {
      if (!fst->InputSymbols()) {
        LOG(WARNING) << "LoadFstFromFar: FLAGS_save_symbols is set "
                     << "but fst has no input symbols";
      }
      if (!fst->OutputSymbols()) {
        LOG(WARNING) << "LoadFstFromFar: FLAGS_save_symbols is set "
                     << "but fst has no output symbols";
      }
    }
    return std::make_unique<DataType>(std::move(fst));
  }

 private:
  LoadFstFromFar<Arc>(const LoadFstFromFar<Arc>&) = delete;
  LoadFstFromFar<Arc>& operator=(const LoadFstFromFar<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_LOADFSTFROMFAR_H_
