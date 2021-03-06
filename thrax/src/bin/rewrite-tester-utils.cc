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
#include <../bin/rewrite-tester-utils.h>

#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <thrax/compat/utils.h>
#include <fst/arc.h>
#include <fst/fst.h>
#include <fst/string.h>
#include <fst/symbol-table.h>
#include <fst/vector-fst.h>
#include <thrax/grm-manager.h>
#include <../bin/utildefs.h>
#include <thrax/symbols.h>
#define HISTORY_FILE ".rewrite-tester-history"
#ifdef HAVE_READLINE
using thrax::File;
using thrax::Open;
#endif  // HAVE_READLINE

using ::fst::StdArc;
using ::fst::StdVectorFst;
using ::fst::StringCompiler;
using ::fst::SymbolTable;
using ::fst::TokenType;
using ::thrax::FstToStrings;
using ::thrax::GetGeneratedSymbolTable;
using ::thrax::RuleTriple;

DEFINE_string(far, "", "Path to the FAR.");
DEFINE_string(rules, "", "Names of the rewrite rules.");
DEFINE_string(input_mode, "byte", "Either \"byte\", \"utf8\", or the path to a "
              "symbol table for input parsing.");
DEFINE_string(output_mode, "byte", "Either \"byte\", \"utf8\", or the path to "
              "a symbol table for input parsing.");
DEFINE_string(history_file, HISTORY_FILE,
              "Location of history file");
DEFINE_int64(noutput, 1, "Maximum number of output strings for each input.");
DEFINE_bool(show_details, false, "Show the output of each individual rule when"
            " multiple rules are specified.");

#ifdef HAVE_READLINE
using thrax::File;
using thrax::Open;
static bool kHistoryFileInitialized = false;

inline void InitializeHistoryFile() {
  if (FST_FLAGS_history_file.empty()) {
    // Doesn't mean it succeeded: just means don't try this again:
    kHistoryFileInitialized = true;
    return;
  }
  // Creates history file if it doesn't exist.
  if (!Open(FST_FLAGS_history_file, "r")) {
    File* fp = Open(FST_FLAGS_history_file, "w");
    // Fails silently if we can't open it: just don't record history.
    if (fp) fp->Close();
  }
  // This will fail silently if history_file doesn't open.
  read_history(FST_FLAGS_history_file.c_str());
  // Doesn't mean it succeeded: just means don't try this again.
  kHistoryFileInitialized = true;
}

bool RewriteTesterUtils::ReadInput(std::string* s) {
  if (!kHistoryFileInitialized) InitializeHistoryFile();
  char* input = readline("Input string: ");
  if (!input) return false;
  s->assign(input);
  if (!FST_FLAGS_history_file.empty()) add_history(input);
  free(input);
  if (!FST_FLAGS_history_file.empty())
    write_history(FST_FLAGS_history_file.c_str());
  return true;
}
#else   // HAVE_READLINE
bool RewriteTesterUtils::ReadInput(std::string* s) {
  std::cout << "Input string: ";
  return static_cast<bool>(getline(std::cin, *s));
}
#endif  // HAVE_READLINE

RewriteTesterUtils::RewriteTesterUtils() :
    compiler_(nullptr),
    byte_symtab_(nullptr),
    utf8_symtab_(nullptr),
    input_symtab_(nullptr),
    output_symtab_(nullptr)  { }

void RewriteTesterUtils::Initialize() {
  CHECK(grm_.LoadArchive(FST_FLAGS_far));
  rules_ = ::fst::StringSplit(FST_FLAGS_rules, ',');
  byte_symtab_ = nullptr;
  utf8_symtab_ = nullptr;
  if (rules_.empty()) LOG(FATAL) << "--rules must be specified";
  for (size_t i = 0; i < rules_.size(); ++i) {
    RuleTriple triple(rules_[i]);
    const auto *fst = grm_.GetFst(triple.main_rule);
    if (!fst) {
      LOG(FATAL) << "grm.GetFst() must be non nullptr for rule: "
                 << triple.main_rule;
    }
    StdVectorFst vfst(*fst);
    // If the input transducers in the FAR have symbol tables then we need to
    // add the appropriate symbol table(s) to the input strings, according to
    // the parse mode.
    if (vfst.InputSymbols()) {
      if (!byte_symtab_ &&
          vfst.InputSymbols()->Name() ==
              ::thrax::function::kByteSymbolTableName) {
        byte_symtab_ = fst::WrapUnique(vfst.InputSymbols()->Copy());
      } else if (!utf8_symtab_ &&
                 vfst.InputSymbols()->Name() ==
                     ::thrax::function::kUtf8SymbolTableName) {
        utf8_symtab_ = fst::WrapUnique(vfst.InputSymbols()->Copy());
      }
    }
    if (!triple.pdt_parens_rule.empty()) {
      fst = grm_.GetFst(triple.pdt_parens_rule);
      if (!fst) {
        LOG(FATAL) << "grm.GetFst() must be non nullptr for rule: "
                   << triple.pdt_parens_rule;
      }
    }
    if (!triple.mpdt_assignments_rule.empty()) {
      fst = grm_.GetFst(triple.mpdt_assignments_rule);
      if (!fst) {
        LOG(FATAL) << "grm.GetFst() must be non nullptr for rule: "
                   << triple.mpdt_assignments_rule;
      }
    }
  }
  generated_symtab_ = GetGeneratedSymbolTable(&grm_);
  if (FST_FLAGS_input_mode == "byte") {
    compiler_ = std::make_unique<StringCompiler<StdArc>>(TokenType::BYTE);
  } else if (FST_FLAGS_input_mode == "utf8") {
    compiler_ = std::make_unique<StringCompiler<StdArc>>(TokenType::UTF8);
  } else {
    input_symtab_ = fst::WrapUnique(
        SymbolTable::ReadText(FST_FLAGS_input_mode));
    if (!input_symtab_) {
      LOG(FATAL) << "Invalid mode or symbol table path.";
    }
    compiler_ = std::make_unique<StringCompiler<StdArc>>(TokenType::SYMBOL,
                                                          input_symtab_.get());
  }
  output_symtab_ = nullptr;
  if (FST_FLAGS_output_mode == "byte") {
    type_ = TokenType::BYTE;
  } else if (FST_FLAGS_output_mode == "utf8") {
    type_ = TokenType::UTF8;
  } else {
    type_ = TokenType::SYMBOL;
    output_symtab_ = fst::WrapUnique(
        SymbolTable::ReadText(FST_FLAGS_output_mode));
    if (!output_symtab_) {
      LOG(FATAL) << "Invalid mode or symbol table path.";
    }
  }
}

const std::string RewriteTesterUtils::ProcessInput(const std::string& input,
                                                   bool prepend_output) {
  StdVectorFst input_fst;
  StdVectorFst output_fst;
  if (!compiler_->operator()(input, &input_fst)) {
    return "Unable to parse input string.";
  }
  std::ostringstream sstrm;
  // Set symbols for the input, if appropriate
  if (byte_symtab_ && type_ == TokenType::BYTE) {
    input_fst.SetInputSymbols(byte_symtab_.get());
    input_fst.SetOutputSymbols(byte_symtab_.get());
  } else if (utf8_symtab_ && type_ == TokenType::UTF8) {
    input_fst.SetInputSymbols(utf8_symtab_.get());
    input_fst.SetOutputSymbols(utf8_symtab_.get());
  } else if (input_symtab_ && type_ == TokenType::SYMBOL) {
    input_fst.SetInputSymbols(input_symtab_.get());
    input_fst.SetOutputSymbols(input_symtab_.get());
  }
  bool succeeded = true;
  for (size_t i = 0; i < rules_.size(); ++i) {
    RuleTriple triple(rules_[i]);
    if (grm_.Rewrite(triple.main_rule, input_fst, &output_fst,
                     triple.pdt_parens_rule, triple.mpdt_assignments_rule)) {
      if (FST_FLAGS_show_details && rules_.size() > 1) {
        std::vector<std::pair<std::string, float>> tmp;
        FstToStrings(output_fst, &tmp, generated_symtab_.get(), type_,
                     output_symtab_.get(), FST_FLAGS_noutput);
        for (const auto& one_result : tmp) {
          sstrm << "output of rule[" << triple.main_rule
                << "] is: " << one_result.first << '\n';
        }
      }
      input_fst = output_fst;
    } else {
      succeeded = false;
      break;
    }
  }
  std::vector<std::pair<std::string, float>> strings;
  std::set<std::string> seen;
  if (succeeded &&
      FstToStrings(output_fst, &strings, generated_symtab_.get(), type_,
                   output_symtab_.get(), FST_FLAGS_noutput)) {
    for (auto it = strings.cbegin(); it != strings.cend(); ++it) {
      const auto sx = seen.find(it->first);
      if (sx != seen.end()) continue;
      if (prepend_output) {
        sstrm << "Output string: " << it->first;
      } else {
        sstrm << it->first;
      }
      if (FST_FLAGS_noutput != 1 && it->second != 0) {
        sstrm << " <cost=" << it->second << '>';
      }
      seen.insert(it->first);
      if (it + 1 != strings.cend()) sstrm << '\n';
    }
    return sstrm.str();
  } else {
    return "Rewrite failed.";
  }
}

// Run() for interactive mode.
void RewriteTesterUtils::Run() {
  std::string input;
  while (ReadInput(&input)) std::cout << ProcessInput(input) << std::endl;
}
