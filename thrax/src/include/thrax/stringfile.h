// Reads in a file of strings to be compiled into an FST using a prefix tree.

#ifndef THRAX_STRINGFILE_H_
#define THRAX_STRINGFILE_H_

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <fst/mutable-fst.h>
#include <fst/properties.h>
#include <fst/push.h>
#include <fst/rmepsilon.h>
#include <fst/string.h>
#include <fst/symbol-table.h>
#include <thrax/algo/stringmap.h>
#include <thrax/fst-node.h>
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
  using TransducerPrefixTree = ::fst::TransducerPrefixTree<Arc>;
  using Label = typename Arc::Label;

  StringFile() {}
  ~StringFile() final {}

 protected:
  DataType* Execute(const std::vector<DataType*>& args) final {
    if (args.size() < 1 || args.size() > 3) {
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
      // FLAGS_save_symbols is set, we should set the symbol table to byte
      // mode.
      if (FLAGS_save_symbols) isymbols = GetByteSymbolTable();
    } else if (args.size() > 1) {
      if (args[1]->is<std::string>()) {
        if (*args[1]->get<std::string>() == "utf8") {
          imode = ::fst::TokenType::UTF8;
          if (FLAGS_save_symbols) isymbols = GetUtf8SymbolTable();
        } else {
          imode = ::fst::TokenType::BYTE;
          if (FLAGS_save_symbols) isymbols = GetByteSymbolTable();
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
          if (FLAGS_save_symbols) osymbols = GetUtf8SymbolTable();
        } else {
          omode = ::fst::TokenType::BYTE;
          if (FLAGS_save_symbols) osymbols = GetByteSymbolTable();
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
    const auto filename =
        JoinPath(FLAGS_indir, *args[0]->get<std::string>());
    auto fst = fst::make_unique<MutableTransducer>();
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
    if (FLAGS_save_symbols) {
      fst->SetInputSymbols(isymbols);
      fst->SetOutputSymbols(osymbols);
    }
    return new DataType(fst.release());
  }
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_STRINGFILE_H_
