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
// Creates a string FST given the string text.
//
// Note that error checking in this function may be harsher or missing, as this
// function cannot be called natively by the user but must go through the
// parser.

#ifndef THRAX_STRINGFST_H_
#define THRAX_STRINGFST_H_

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/string.h>
#include <thrax/algo/stringcompile.h>
#include <thrax/fst-node.h>
#include <thrax/datatype.h>
#include <thrax/function.h>
#include <thrax/symbols.h>

DECLARE_bool(save_symbols);  // From util/flags.cc.
DECLARE_string(indir);  // From util/flags.cc.

namespace thrax {
namespace function {

template <typename Arc>
class StringFst : public Function<Arc> {
 public:
  using MutableTransducer = ::fst::VectorFst<Arc>;

  StringFst() {}
  ~StringFst() final {}

 protected:
  std::unique_ptr<DataType> Execute(
      const std::vector<std::unique_ptr<DataType>>& args) final {
    CHECK_GE(args.size(), 2);
    // Find the mode (and maybe the symbol table).
    auto mode = ::fst::TokenType::BYTE;
    const ::fst::SymbolTable* symtab = nullptr;
    switch (*args[0]->get<int>()) {
      case StringFstNode::BYTE: {
        CHECK_EQ(args.size(), 2);
        mode = ::fst::TokenType::BYTE;
        break;
      }
      case StringFstNode::UTF8: {
        CHECK_EQ(args.size(), 2);
        mode = ::fst::TokenType::UTF8;
        break;
      }
      case StringFstNode::SYMBOL_TABLE: {
        CHECK_EQ(args.size(), 3);
        mode = ::fst::TokenType::SYMBOL;
        if (!args[2]->is<::fst::SymbolTable>()) {
          std::cout
              << "StringFst: Invalid symbol table for symbol table parse mode"
              << std::endl;
          return nullptr;
        }
        symtab = args[2]->get<::fst::SymbolTable>();
        break;
      }
      default: {
        LOG(FATAL) << "Unhandled parse mode.";
      }
    }
    // Go through the actual text and process each block, escaping the
    // backslashed characters and generating labels if necessary.
    const auto& text = *args[1]->get<std::string>();
    auto fst = std::make_unique<MutableTransducer>();
    if (!::fst::StringCompile(text, fst.get(), mode, symtab)) {
      std::cout << "StringFst: Failed to compile string: " << text << std::endl;
      return nullptr;
    }
    if (FLAGS_save_symbols) {
      const ::fst::SymbolTable* syms_to_attach = nullptr;
      switch (mode) {
        case ::fst::TokenType::BYTE: {
          syms_to_attach = GetByteSymbolTable();
          break;
        }
        case ::fst::TokenType::UTF8: {
          syms_to_attach = GetUtf8SymbolTable();
          break;
        }
        case ::fst::TokenType::SYMBOL: {
          syms_to_attach = symtab;
        }
      }
      fst->SetInputSymbols(syms_to_attach);
      fst->SetOutputSymbols(syms_to_attach);
    }
    return std::make_unique<DataType>(std::move(fst));
  }

 public:
  // Returns a symbol table corresponding to the generated labels used thus far
  // in this compilation. This returns nullptr if there were no generated
  // labels.
  //
  // If FLAGS_save_symbols is set, we also add these labels to the byte and utf8
  // symbol tables, so that these can get reassigned to the transducers as
  // appropriate on write-out. However, we only want to do this is this is a
  // top-level grammar and we are saving out the FARs for this grammar (i.e.,
  // if it is not being imported into another top-level grammar). The boolean
  // argument top_level controls this.
  static std::unique_ptr<::fst::SymbolTable> GetLabelSymbolTable(
      bool top_level) {
    // TODO(wolfsonkin): Don't copy the generated symbols, though note that this
    // isn't dangerous as all users of this function don't hold on to the symbol
    // table and expect it to not be current forever. Instead, give a const ref.
    auto symtab = fst::WrapUnique(::fst::GeneratedSymbols().Copy());

    // NB: We return a nullptr if the generated symbol table has never had
    // anything added to it outside its defaults, as consumers of this function
    // use that as an indicator of whether to save the SymbolTable into the
    // created FAR.
    if (symtab->AvailableKey() <= 1) return nullptr;
    if (FLAGS_save_symbols && top_level) {
      for (const auto& item : *symtab) {
        const auto& symbol = item.Symbol();
        const auto key = item.Label();
        AddToByteSymbolTable(symbol, key);
        AddToUtf8SymbolTable(symbol, key);
      }
    }
    return symtab;
  }

  // This takes in a symbol table and merges it into the current symbol/label
  // map, returning true on success or failure if we encounter any conflicts.
  static bool MergeLabelSymbolTable(const ::fst::SymbolTable& symtab) {
    return ::fst::thrax_internal::MergeIntoGeneratedSymbols(symtab,
                                                                &remap_);
  }

  static void ClearRemap() { remap_.clear(); }

  // Returns the remap value, or ::fst::kNoLabel
  static int64 FindRemapLabel(int64 old_label) {
    const auto it = remap_.find(old_label);
    return it == remap_.end() ? ::fst::kNoLabel : it->second;
  }

  // This stores the assigned label for the provided symbol (from the map) into
  // label and returns true on a successful lookup or false if the symbol isn't
  // found.
  static bool SymbolToGeneratedLabel(const std::string& symbol, int64* label) {
    const int64 answer = ::fst::GeneratedSymbols().Find(symbol);
    if (answer == ::fst::kNoSymbol) return false;
    *label = answer;
    return true;
  }

 private:
  // This function clears the static map for symbols to labels. This is just to
  // clean up between test runs. This is somewhat less appropriately named than
  // it used to be since it clears more than just the generated SymbolTable.
  static void ClearSymbolLabelMapForTest() {
    remap_.clear();
    ::fst::thrax_internal::ResetGeneratedSymbols();
  }

  static std::map<int64, int64> remap_;

  friend class CategoryTest;
  friend class FeatureTest;
  friend class FeatureVectorTest;
  friend class StringFstTest;

  StringFst<Arc>(const StringFst<Arc>&) = delete;
  StringFst<Arc>& operator=(const StringFst<Arc>&) = delete;
};

template <typename Arc>
typename std::map<int64, int64> StringFst<Arc>::remap_;

}  // namespace function
}  // namespace thrax

#endif  // THRAX_STRINGFST_H_
