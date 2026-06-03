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

// Loads the appropriate symbol table given the string.

#ifndef OPENGRM_THRAX_WALKER_UTIL_FUNCTION_SYMBOLTABLE_H_
#define OPENGRM_THRAX_WALKER_UTIL_FUNCTION_SYMBOLTABLE_H_

#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "openfst/lib/symbol-table.h"
#include "opengrm/thrax/walker/util/datatype.h"
#include "opengrm/thrax/walker/util/function/function.h"

ABSL_DECLARE_FLAG(std::string, indir);  // From util/flags.cc.

namespace thrax {
namespace function {

template <typename Arc>
class SymbolTable : public Function<Arc> {
 public:
  SymbolTable() = default;
  ~SymbolTable() final = default;

 protected:
  std::unique_ptr<DataType> Execute(
      const std::vector<std::unique_ptr<DataType>>& args) const final {
    if (args.size() != 1) {
      std::cout << "SymbolTable: Expected 1 argument but got " << args.size()
                << std::endl;
      return nullptr;
    }
    if (!args[0]->is<std::string>()) {
      std::cout << "SymbolTable: Expected string (path) for argument 1"
                << std::endl;
      return nullptr;
    }
    const auto& file = ::fst::JoinPath(absl::GetFlag(FLAGS_indir),
                                        *args[0]->get<std::string>());
    VLOG(2) << "Loading symbol table: " << file;
    std::unique_ptr<::fst::SymbolTable> symtab(
        ::fst::SymbolTable::ReadText(file));
    if (!symtab) {
      std::cout << "SymbolTable: Unable to load symbol table file: " << file
                << std::endl;
      return nullptr;
    }
    return std::make_unique<DataType>(*symtab);
  }

 private:
  SymbolTable(const SymbolTable<Arc>&) = delete;
  SymbolTable<Arc>& operator=(const SymbolTable<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // OPENGRM_THRAX_WALKER_UTIL_FUNCTION_SYMBOLTABLE_H_
