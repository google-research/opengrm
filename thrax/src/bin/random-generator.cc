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
// Stand-alone binary to load up a FAR and generate a random set of strings from
// a given rule. Useful for debugging to see the kinds of things the grammar
// rule will accept.

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/compat/utils.h>
#include <fst/arc.h>
#include <fst/fst.h>
#include <fst/randgen.h>
#include <fst/string.h>
#include <fst/symbol-table.h>
#include <fst/vector-fst.h>
#include <thrax/grm-manager.h>
#include <../bin/utildefs.h>
#include <thrax/symbols.h>

using ::fst::Project;
using ::fst::ProjectType;
using ::fst::RandGen;
using ::fst::RandGenOptions;
using ::fst::StdArc;
using ::fst::StdVectorFst;
using ::fst::SymbolTable;
using ::fst::TokenType;
using ::fst::UniformArcSelector;
using ::thrax::FstToStrings;
using ::thrax::GetGeneratedSymbolTable;
using ::thrax::GrmManagerSpec;

DEFINE_string(far, "", "Path to the FAR");
DEFINE_string(rule, "", "Names of the rewrite rule");
DEFINE_string(input_mode, "byte",
              "Either \"byte\", \"utf8\", or the path to "
              "a symbol table for input parsing");
DEFINE_string(output_mode, "byte",
              "Either \"byte\", \"utf8\", or the path to "
              "a symbol table for input parsing");
DEFINE_int64(noutput, 1, "Maximum number of output mappings to generate");

int main(int argc, char** argv) {
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(argv[0], &argc, &argv, true);

  GrmManagerSpec<StdArc> grm;
  CHECK(grm.LoadArchive(FST_FLAGS_far));
  std::unique_ptr<SymbolTable> generated_symtab = GetGeneratedSymbolTable(&grm);
  std::unique_ptr<SymbolTable> output_symtab;
  TokenType type;
  if (FST_FLAGS_output_mode == "byte") {
    type = TokenType::BYTE;
  } else if (FST_FLAGS_output_mode == "utf8") {
    type = TokenType::UTF8;
  } else {
    type = TokenType::SYMBOL;
    output_symtab.reset(
        SymbolTable::ReadText(FST_FLAGS_output_mode));
    if (!output_symtab) {
      LOG(FATAL) << "Invalid mode or symbol table path";
    }
  }
  std::unique_ptr<SymbolTable> input_symtab;
  if (FST_FLAGS_input_mode == "byte") {
    type = TokenType::BYTE;
  } else if (FST_FLAGS_input_mode == "utf8") {
    type = TokenType::UTF8;
  } else {
    type = TokenType::SYMBOL;
    input_symtab.reset(SymbolTable::ReadText(FST_FLAGS_input_mode));
    if (!input_symtab) {
      LOG(FATAL) << "Invalid mode or symbol table path";
    }
  }
  if (FST_FLAGS_rule.empty()) {
    LOG(FATAL) << "--rule must be specified";
  }
  const auto* fst = grm.GetFst(FST_FLAGS_rule);
  if (!fst) {
    LOG(FATAL) << "grm.GetFst() must be non nullptr for rule: "
               << FST_FLAGS_rule;
  }

  // If the exported rule is not optimized, it may have final Infinite
  // costs. This can cause problems with randgen. RmEpsilon has the effect of
  // cleaning this up.
  StdVectorFst cleaned(*fst);
  RmEpsilon(&cleaned);
  std::vector<std::pair<std::string, float>> istrings;
  std::vector<std::pair<std::string, float>> ostrings;

  ::UniformArcSelector<StdArc> uniform_selector;
  const RandGenOptions<UniformArcSelector<StdArc>> opts(
      uniform_selector, /*max_length=*/std::numeric_limits<int32_t>::max(),
      /*npath=*/1, true, false);

  for (int i = 0; i < FST_FLAGS_noutput; ++i) {
    StdVectorFst ofst;
    RandGen(cleaned, &ofst, opts);
    if (ofst.NumStates() == 0) continue;
    StdVectorFst ifst(ofst);
    Project(&ifst, ProjectType::INPUT);
    Project(&ofst, ProjectType::OUTPUT);
    if (!FstToStrings(ifst, &istrings, generated_symtab.get(), type,
                      input_symtab.get())) {
      LOG(FATAL) << "Can't generate strings for input side";
    }
    if (!FstToStrings(ofst, &ostrings, generated_symtab.get(), type,
                      output_symtab.get())) {
      LOG(FATAL) << "Can't generate strings for output side";
    }
  }
  for (int i = 0; i < istrings.size(); ++i) {
    std::cout << "****************************************" << std::endl;
    std::cout << istrings[i].first << std::endl << ostrings[i].first
              << std::endl;
    // TODO(rws): Currently there is an issue that RandGen() removes weights, so
    // we'll never actually see these costs.
    if (istrings[i].second != 0) {
      std::cout << " <cost=" << istrings[i].second << ">" << std::endl;
    }
  }

  return 0;
}
