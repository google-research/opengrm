// Creates a string FST given the string text.
//
// Note that error checking in this function may be harsher or missing, as this
// function cannot be called natively by the user but must go through the
// parser.

#ifndef THRAX_STRINGFST_H_
#define THRAX_STRINGFST_H_

#include <stdlib.h>

#include <cctype>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/fstlib.h>
#include <fst/string.h>
#include <thrax/fst-node.h>
#include <thrax/datatype.h>
#include <thrax/function.h>
#include <thrax/symbols.h>
#include <thrax/compat/stlfunctions.h>

DECLARE_bool(save_symbols);  // From util/flags.cc.
DECLARE_string(indir);  // From util/flags.cc.

namespace thrax {
namespace function {

// Default values here correspond to Unicode Private Use Area B (Plane 16):
// 100000 - 10FFFB. The last two positions are reserved for BOS and EOS symbols.
constexpr int64 kGeneratedLabelStartIndex = 0x100000;
constexpr int64 kGeneratedLabelEndIndex = 0x10fffb;

constexpr char kInitialBoundarySymbol[] = "BOS";
constexpr int64 kInitialBoundaryLabel = 0x10fffc;
constexpr char kFinalBoundarySymbol[] = "EOS";
constexpr int64 kFinalBoundaryLabel = 0x10fffd;

template <typename Arc>
class StringFst : public Function<Arc> {
 public:
  using MutableTransducer = ::fst::VectorFst<Arc>;

  StringFst() {}
  ~StringFst() final {}

 protected:
  DataType* Execute(const std::vector<DataType*>& args) final {
    CHECK_GE(args.size(), 2);
    // Find the mode (and maybe the symbol table).
    auto mode = ::fst::StringTokenType::BYTE;
    const ::fst::SymbolTable* symtab = nullptr;
    switch (*args[0]->get<int>()) {
      case StringFstNode::BYTE: {
        CHECK_EQ(args.size(), 2);
        mode = ::fst::StringTokenType::BYTE;
        break;
      }
      case StringFstNode::UTF8: {
        CHECK_EQ(args.size(), 2);
        mode = ::fst::StringTokenType::UTF8;
        break;
      }
      case StringFstNode::SYMBOL_TABLE: {
        CHECK_EQ(args.size(), 3);
        mode = ::fst::StringTokenType::SYMBOL;
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
    const ::fst::StringCompiler<Arc> compiler(mode, symtab);
    // Create a single state acceptor to serve as the basis onto which we'll
    // concatenate everything.
    std::unique_ptr<MutableTransducer> fst(new MutableTransducer());
    fst->SetStart(fst->AddState());
    fst->SetFinal(fst->Start(), Arc::Weight::One());
    // Go through the actual text and process each block, escaping the
    // backslashed characters and generating labels if necessary.
    const auto& text = *args[1]->get<std::string>();
    // Either the raw text to be added or the name of a symbol we wish to
    // generate a label for.
    std::string chunk;
    // Whether we're currently in generate-label mode.
    bool in_genlab = false;
    for (size_t i = 0; i < text.length(); ++i) {
      char c = text[i];
      if (c == '[' && mode != ::fst::StringTokenType::SYMBOL) {
        if (in_genlab) {
          std::cout << "StringFst: Cannot start new generated label while in "
                    << "previous label" << std::endl;
          return nullptr;
        }
        if (!AddBlock(compiler, &chunk, fst.get())) {
          std::cout << "StringFst: Failed to compile chunk: " << chunk
                    << std::endl;
          return nullptr;
        }
        in_genlab = true;
      } else if (c == ']' && mode != ::fst::StringTokenType::SYMBOL) {
        if (!in_genlab) {
          std::cout
              << "StringFst: Cannot terminate generated label without already "
              << "being in one" << std::endl;
          return nullptr;
        }
        for (const auto ch : chunk) {
          if (isspace(ch)) {
            std::cout << "StringFst: Cannot have labels containing whitespace: "
                      << chunk << std::endl;
            return nullptr;
          }
        }
        if (!AddGeneratedLabel(&chunk, fst.get(), mode)) {
          std::cout << "StringFst: Failed to generate label: " << chunk
                    << std::endl;
          return nullptr;
        }
        in_genlab = false;
      } else {
        // Here, we're just going to add a single character to the current
        // chunk.
        if (c == '\\') {
          if (++i >= text.length()) {
            std::cout
                << "StringFst: Unterminated escaped character at string end"
                << std::endl;
            return nullptr;
          }
          c = text[i];
          switch (c) {
            case 'n': {
              c = '\n';
              break;
            }
            case 'r': {
              c = '\r';
              break;
            }
            case 't': {
              c = '\t';
              break;
            }
          }
        }
        chunk += c;
      }
    }
    if (in_genlab) {
      std::cout << "StringFst: Unexpected string termination in generated label"
                << std::endl;
      return nullptr;
    }
    if (!AddBlock(compiler, &chunk, fst.get())) {
      std::cout << "StringFst: Failed to compile chunk: " << chunk << std::endl;
      return nullptr;
    }
    ::fst::RmEpsilon(fst.get());
    if (FLAGS_save_symbols) {
      if (symtab) {
        fst->SetInputSymbols(symtab);
        fst->SetOutputSymbols(symtab);
      } else if (mode == ::fst::StringTokenType::UTF8) {
        fst->SetInputSymbols(GetUtf8SymbolTable());
        fst->SetOutputSymbols(GetUtf8SymbolTable());
      } else {
        fst->SetInputSymbols(GetByteSymbolTable());
        fst->SetOutputSymbols(GetByteSymbolTable());
      }
    }
    return new DataType(fst.release());
  }

 public:
  // Returns a symbol table corresponding to the generated labels used thus far
  // in this compilation. This returns nullptr if there were no generated
  // labels.
  //
  // The caller takes ownership.
  //
  // If FLAGS_save_symbols is set, we also add these labels to the byte and utf8
  // symbol tables, so that these can get reassigned to the transducers as
  // appropriate on write-out. However, we only want to do this is this is a
  // top-level grammar and we are saving out the FARs for this grammar (i.e.,
  // if it is not being imported into another top-level grammar). The boolean
  // argument top_level controls this.
  static ::fst::SymbolTable* GetLabelSymbolTable(bool top_level) {
    ::fst::MutexLock lock(&map_mutex_);
    if (symbol_label_map_.empty()) return nullptr;
    auto* symtab = new ::fst::SymbolTable();
    for (auto it = symbol_label_map_.begin(); it != symbol_label_map_.end();
         ++it) {
      const auto& symbol = it->first;
      const auto key = it->second;
      symtab->AddSymbol(symbol, key);
      if (FLAGS_save_symbols && top_level) {
        AddToByteSymbolTable(symbol, key);
        AddToUtf8SymbolTable(symbol, key);
      }
    }
    return symtab;
  }

  // This takes in a symbol table and merges it into the current symbol/label
  // map, returning true on success or failure if we encounter any conflicts.
  static bool MergeLabelSymbolTable(const ::fst::SymbolTable& symtab) {
    ::fst::MutexLock lock(&map_mutex_);
    bool success = true;
    for (int i = 0; i < symtab.NumSymbols(); ++i) {
      int64 label = symtab.GetNthKey(i);
      std::string symbol = symtab.Find(label);
      // Checks to see if we already have this label paired with this
      // symbol. FSTs associated with the incoming symbol table will get
      // remapped as needed.
      // Four possible outcomes:
      // 1) Neither label nor symbol exist: insert this new pair.
      // 2) Label exists but mapped to another symbol: generate new_label for
      //    the symbol, and add <label, new_label> to the remapping table.
      // 3) Symbol exists but with another old_label: reassign to old_label and
      //    add <label, old_label> to the remapping table.
      // 4) Both label and symbol exist: then we need to ask whether they have
      //    the same mapping.
      const auto slx = symbol_label_map_.find(symbol);
      const auto lsx = label_symbol_map_.find(label);
      if (slx == symbol_label_map_.end() && lsx == label_symbol_map_.end()) {
        // Case 1: Both new
        symbol_label_map_.emplace(symbol, label);
        label_symbol_map_.emplace(label, symbol);
        VLOG(2) << "Loaded symbol " << symbol << " with label " << label;
        // On success, keeps track of the maximum + 1 for the next available
        // label.
        if (next_label_ <= label) next_label_ = label + 1;
      } else if (slx == symbol_label_map_.end()) {
        // Case 2: symbol is new, but label is there and therefore mapped to
        // something else.
        int64 new_label = next_label_++;
        symbol_label_map_.emplace(symbol, new_label);
        label_symbol_map_.emplace(new_label, symbol);
        remap_.emplace(label, new_label);
        VLOG(2) << "Remapping " << symbol << " to new label " << new_label;
      } else if (lsx == label_symbol_map_.end()) {
        // Case 3: label is new, but symbol is there and therefore mapped to
        // something else.
        int64 old_label = slx->second;
        remap_.emplace(label, old_label);
        VLOG(2) << "Remapping " << symbol << " to old label " << old_label;
      } else {
        // Case 4: Both symbol and label already exist.
        const std::string& old_symbol = lsx->second;
        int64 old_label = slx->second;
        if (symbol == old_symbol && label == old_label) {
          // Same, so ok and nothing to do.
          continue;
        } else if (label == old_label || symbol == old_symbol) {
          // symbol -> label gets you the right label, but label -> symbol
          // doesn't, or vice versa. This should not happen.
          LOG(WARNING) << "Detected label mismatch: "
                       << symbol << " -> " << old_label << ", "
                       << label << " -> " << old_symbol;
          success = false;
        } else {
          // Both are there but assigned to other things.
          remap_.emplace(label, old_label);
          VLOG(2) << "Remapping " << symbol << " to old label " << old_label;
        }
      }
    }
    return success;
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
    int64* answer = FindOrNull(symbol_label_map_, symbol);
    if (!answer) return false;
    *label = *answer;
    return true;
  }

 private:
  bool AddBlock(const ::fst::StringCompiler<Arc>& compiler,
                std::string* chunk, MutableTransducer* fst) {
    VLOG(3) << "Adding block: " << *chunk;
    MutableTransducer chunk_fst;
    if (!compiler(*chunk, &chunk_fst)) return false;
    ::fst::Concat(fst, chunk_fst);
    chunk->clear();
    return true;
  }

  static void SingleCharacterError(const std::string& symbol) {
    LOG(ERROR) << "Single-character generated labels are not permitted. "
               << "Replace \"[" << symbol << "]\" with just \"" << symbol
               << "\" in your grammar." << std::endl;
  }

  bool AddGeneratedLabel(std::string* symbol, MutableTransducer* fst,
                         enum ::fst::StringTokenType mode) {
    VLOG(3) << "Finding label for symbol: " << *symbol;
    int64 label;
    // First, we'll check to see if the symbol is actually just a number. If
    // so, we'll just use that number as the label.
    char* remainder = nullptr;
    label = strtol(symbol->c_str(), &remainder, 0);
    // So if we're at the end of the string, then we've found a valid (whole)
    // number. Otherwise (i.e., in this case) we have other junk, so we'll
    // treat it as a generated text symbol.
    if (*remainder != '\0') {
      // First of all check to see if the symbol is one of the designated
      // beginning-of-string/end-of-string delimiters. We don't put these in the
      // symbol table, but hallucinate them on the fly:
      if (*symbol == kInitialBoundarySymbol) {
        label = kInitialBoundaryLabel;
      } else if (symbol->size() == 1) {
        // Single-byte bracketed character.
        StringFst<Arc>::SingleCharacterError(*symbol);
        return false;
      } else if (mode == ::fst::StringTokenType::UTF8) {
        std::vector<int64> labels;
        if (!::fst::UTF8StringToLabels(*symbol, &labels)) return false;
        // Single Unicode codepoint-bracketed character.
        if (labels.size() == 1) {
          StringFst<Arc>::SingleCharacterError(*symbol);
          return false;
        }
      } else if (*symbol == kFinalBoundarySymbol) {
        label = kFinalBoundaryLabel;
      } else {
        // Find the right label - either the one previously generated for the
        // symbol, or a new one that we'll cache now.
        const auto result = symbol_label_map_.emplace(*symbol, next_label_);
        if (result.second) {
          label = next_label_++;
          label_symbol_map_.emplace(label, *symbol);
          VLOG(2) << "Assigned symbol " << *symbol << " to label " << label;
        } else {
          label = result.first->second;
        }
        if (label > kGeneratedLabelEndIndex) {
          LOG(WARNING) << "Exceeded maximum number of generated labels.";
          return false;
        }
      }
    }
    // Create a machine with a single arc using that label.
    MutableTransducer genlab_fst;
    const auto p = genlab_fst.AddState();
    const auto q = genlab_fst.AddState();
    genlab_fst.SetStart(p);
    genlab_fst.EmplaceArc(p, label, label, q);
    genlab_fst.SetFinal(q, Arc::Weight::One());
    ::fst::Concat(fst, genlab_fst);
    symbol->clear();
    return true;
  }

  // This function clears the static map for symbols to labels. This is just to
  // clean up between test runs. This is somewhat less appropriately named than
  // it used to be since it clears more than just the symbol_label_map_
  static void ClearSymbolLabelMapForTest() {
    ::fst::MutexLock lock(&map_mutex_);
    symbol_label_map_.clear();
    label_symbol_map_.clear();
    remap_.clear();
    next_label_ = kGeneratedLabelStartIndex;
  }

  static std::map<std::string, int64> symbol_label_map_;
  static std::map<int64, std::string> label_symbol_map_;
  static std::map<int64, int64> remap_;
  static int64 next_label_;
  static ::fst::Mutex map_mutex_;

  friend class CategoryTest;
  friend class FeatureTest;
  friend class FeatureVectorTest;
  friend class StringFstTest;

  StringFst<Arc>(const StringFst<Arc>&) = delete;
  StringFst<Arc>& operator=(const StringFst<Arc>&) = delete;
};

template <typename Arc>
typename std::map<std::string, int64> StringFst<Arc>::symbol_label_map_;

template <typename Arc>
typename std::map<int64, std::string> StringFst<Arc>::label_symbol_map_;

template <typename Arc>
typename std::map<int64, int64> StringFst<Arc>::remap_;

template <typename Arc>
int64 StringFst<Arc>::next_label_ = kGeneratedLabelStartIndex;

template <typename Arc>
 ::fst::Mutex StringFst<Arc>::map_mutex_;

}  // namespace function
}  // namespace thrax

#endif  // THRAX_STRINGFST_H_
