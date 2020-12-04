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
// Loads the appropriate symbol table given the string.

#ifndef THRAX_SYMBOLTABLE_H_
#define THRAX_SYMBOLTABLE_H_

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/compat/utils.h>
#include <fst/symbol-table.h>
#include <thrax/datatype.h>
#include <thrax/function.h>

DECLARE_string(indir);  // From util/flags.cc.

namespace thrax {
namespace function {

template <typename Arc>
class SymbolTable : public Function<Arc> {
 public:
  SymbolTable() {}
  ~SymbolTable() final {}

 protected:
  std::unique_ptr<DataType> Execute(
      const std::vector<std::unique_ptr<DataType>>& args) final {
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
    const auto& file =
        JoinPath(FLAGS_indir, *args[0]->get<std::string>());
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
  SymbolTable<Arc>(const SymbolTable<Arc>&) = delete;
  SymbolTable<Arc>& operator=(const SymbolTable<Arc>&) = delete;
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_SYMBOLTABLE_H_
