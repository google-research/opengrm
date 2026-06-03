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

// Reads in a file of strings to be compiled into an FST using a prefix tree.

#ifndef OPENGRM_THRAX_WALKER_UTIL_FUNCTION_STRINGFILE_H_
#define OPENGRM_THRAX_WALKER_UTIL_FUNCTION_STRINGFILE_H_

#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "openfst/lib/string.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/string/stringmap.h"
#include "opengrm/thrax/walker/util/datatype.h"
#include "opengrm/thrax/walker/util/function/function.h"
#include "opengrm/thrax/walker/util/function/symbols.h"

ABSL_DECLARE_FLAG(bool, save_symbols);  // From util/flags.cc.
ABSL_DECLARE_FLAG(std::string, indir);  // From util/flags.cc.

namespace thrax {
namespace function {

template <typename Arc>
class StringFile : public Function<Arc> {
 public:
  using MutableTransducer = ::fst::VectorFst<Arc>;
  using Label = typename Arc::Label;

  StringFile() = default;
  ~StringFile() final = default;

 protected:
  std::unique_ptr<DataType> Execute(
      const std::vector<std::unique_ptr<DataType>>& args) const final {
    if (args.empty() || args.size() > 3) {
      std::cout << "StringFile: Expected 1-3 arguments but got " << args.size()
                << std::endl;
      return nullptr;
    }
    if (!args[0]->is<std::string>()) {
      std::cout << "StringFile: Expected string (file) for argument 1"
                << std::endl;
      return nullptr;
    }
    auto imode = ::fst::TokenType::BYTE;
    const ::fst::SymbolTable* isymbols = nullptr;
    if (args.size() == 1) {
      // If the StringFile call doesn't specify a parse mode, but if
      // FLAGS_save_symbols is set, we should set the symbol table to byte
      // mode.
      if (absl::GetFlag(FLAGS_save_symbols)) isymbols = GetByteSymbolTable();
    } else if (args.size() > 1) {
      if (args[1]->is<std::string>()) {
        if (*args[1]->get<std::string>() == "utf8") {
          imode = ::fst::TokenType::UTF8;
          if (absl::GetFlag(FLAGS_save_symbols))
            isymbols = GetUtf8SymbolTable();
        } else {
          imode = ::fst::TokenType::BYTE;
          if (absl::GetFlag(FLAGS_save_symbols))
            isymbols = GetByteSymbolTable();
        }
      } else if (args[1]->is<::fst::SymbolTable>()) {
        isymbols = args[1]->get<::fst::SymbolTable>();
        imode = ::fst::TokenType::SYMBOL;
      } else {
        std::cout << "StringFile: Invalid parse mode or symbol table "
                  << "for input symbols" << std::endl;
        return nullptr;
      }
    }
    auto omode = ::fst::TokenType::BYTE;
    // If this is an acceptor then the output symbols are whatever the input
    // symbols are.
    const ::fst::SymbolTable* osymbols = isymbols;
    if (args.size() > 2) {
      if (args[2]->is<std::string>()) {
        if (*args[2]->get<std::string>() == "utf8") {
          omode = ::fst::TokenType::UTF8;
          if (absl::GetFlag(FLAGS_save_symbols))
            osymbols = GetUtf8SymbolTable();
        } else {
          omode = ::fst::TokenType::BYTE;
          if (absl::GetFlag(FLAGS_save_symbols))
            osymbols = GetByteSymbolTable();
        }
      } else if (args[2]->is<::fst::SymbolTable>()) {
        osymbols = args[2]->get<::fst::SymbolTable>();
        omode = ::fst::TokenType::SYMBOL;
      } else {
        std::cout << "StringFile: Invalid parse mode or symbol table "
                  << "for output symbols" << std::endl;
        return nullptr;
      }
    }
    const auto filename = ::fst::JoinPath(absl::GetFlag(FLAGS_indir),
                                           *args[0]->get<std::string>());
    auto fst = std::make_unique<MutableTransducer>();
    if (!::fst::StringFileCompile(filename, fst.get(), imode, omode, isymbols,
                                  osymbols)) {
      std::cout << "StringFile: File inaccessible or malformed" << std::endl;
      return nullptr;
    }
    if (absl::GetFlag(FLAGS_save_symbols)) {
      fst->SetInputSymbols(isymbols);
      fst->SetOutputSymbols(osymbols);
    }
    return std::make_unique<DataType>(std::move(fst));
  }
};

}  // namespace function
}  // namespace thrax

#endif  // OPENGRM_THRAX_WALKER_UTIL_FUNCTION_STRINGFILE_H_
