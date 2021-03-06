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
// Reads in a file of strings to be compiled into an FST using a prefix tree.

#ifndef THRAX_STRINGFILE_H_
#define THRAX_STRINGFILE_H_

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/compat/utils.h>
#include <fst/arcsort.h>
#include <fst/push.h>
#include <fst/rmepsilon.h>
#include <fst/string.h>
#include <thrax/algo/stringmap.h>
#include <thrax/datatype.h>
#include <thrax/function.h>
#include <thrax/symbols.h>

DECLARE_bool(save_symbols);  // From util/flags.cc.
DECLARE_string(indir);  // From util/flags.cc.

namespace thrax {
namespace function {

template <typename Arc>
class StringFile : public Function<Arc> {
 public:
  using MutableTransducer = ::fst::VectorFst<Arc>;
  using Label = typename Arc::Label;

  StringFile() {}
  ~StringFile() final {}

 protected:
  std::unique_ptr<DataType> Execute(
      const std::vector<std::unique_ptr<DataType>> &args) final {
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
    const ::fst::SymbolTable *isymbols = nullptr;
    if (args.size() == 1) {
      // If the StringFile call doesn't specify a parse mode, but if
      // FST_FLAGS_save_symbols is set, we should set the symbol table to byte
      // mode.
      if (FST_FLAGS_save_symbols) isymbols = GetByteSymbolTable();
    } else if (args.size() > 1) {
      if (args[1]->is<std::string>()) {
        if (*args[1]->get<std::string>() == "utf8") {
          imode = ::fst::TokenType::UTF8;
          if (FST_FLAGS_save_symbols)
            isymbols = GetUtf8SymbolTable();
        } else {
          imode = ::fst::TokenType::BYTE;
          if (FST_FLAGS_save_symbols)
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
    const ::fst::SymbolTable *osymbols = isymbols;
    if (args.size() > 2) {
      if (args[2]->is<std::string>()) {
        if (*args[2]->get<std::string>() == "utf8") {
          omode = ::fst::TokenType::UTF8;
          if (FST_FLAGS_save_symbols)
            osymbols = GetUtf8SymbolTable();
        } else {
          omode = ::fst::TokenType::BYTE;
          if (FST_FLAGS_save_symbols)
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
    const auto filename = JoinPath(FST_FLAGS_indir,
                                           *args[0]->get<std::string>());
    auto fst = std::make_unique<MutableTransducer>();
    if (!::fst::StringFileCompile(filename, fst.get(), imode, omode,
                                      isymbols, osymbols)) {
      std::cout << "StringFile: File inaccessible or malformed" << std::endl;
      return nullptr;
    }
    if (fst->Properties(::fst::kNotAcceptor, true) ==
        ::fst::kNotAcceptor) {
      MutableTransducer copy(*fst);
      ::fst::Push<Arc, ::fst::REWEIGHT_TO_INITIAL>(
          copy, fst.get(), ::fst::kPushLabels);
    }
    ::fst::RmEpsilon(fst.get());
    static const ::fst::ILabelCompare<Arc> icomp;
    ::fst::ArcSort(fst.get(), icomp);
    if (FST_FLAGS_save_symbols) {
      fst->SetInputSymbols(isymbols);
      fst->SetOutputSymbols(osymbols);
    }
    return std::make_unique<DataType>(std::move(fst));
  }
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_STRINGFILE_H_
