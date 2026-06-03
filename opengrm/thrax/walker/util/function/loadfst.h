// Copyright 2026 The OpenGrm Authors.
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

// Loads an FST from the provided filename.

#ifndef OPENGRM_THRAX_WALKER_UTIL_FUNCTION_LOADFST_H_
#define OPENGRM_THRAX_WALKER_UTIL_FUNCTION_LOADFST_H_

#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "absl/memory/memory.h"
#include "openfst/lib/fst.h"
#include "opengrm/thrax/walker/util/datatype.h"
#include "opengrm/thrax/walker/util/function/function.h"

ABSL_DECLARE_FLAG(bool, save_symbols);  // From util/flags.cc.
ABSL_DECLARE_FLAG(std::string, indir);  // From util/flags.cc.

namespace thrax {
namespace function {

template <typename Arc>
class LoadFst : public Function<Arc> {
 public:
  using Transducer = ::fst::Fst<Arc>;

  LoadFst() = default;
  ~LoadFst() final = default;

 protected:
  std::unique_ptr<DataType> Execute(
      const std::vector<std::unique_ptr<DataType>>& args) const final {
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
    const auto& file = ::fst::JoinPath(absl::GetFlag(FLAGS_indir),
                                        *args[0]->get<std::string>());
    VLOG(2) << "Loading FST: " << file;
    auto fst = absl::WrapUnique(Transducer::Read(file));
    if (!fst) {
      std::cout << "LoadFst: Failed to load FST from file: " << file
                << std::endl;
      return nullptr;
    }
    if (absl::GetFlag(FLAGS_save_symbols)) {
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
  LoadFst(const LoadFst<Arc>&) = delete;
  LoadFst<Arc>& operator=(const LoadFst<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // OPENGRM_THRAX_WALKER_UTIL_FUNCTION_LOADFST_H_
