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

// Stand-alone binary to load up a FAR and generate a random set of strings from
// a given rule. Useful for debugging to see the kinds of things the grammar
// rule will accept.

#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/init.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/randgen.h"
#include "openfst/lib/string.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/rewrite/rewrite.h"
#include "opengrm/thrax/grm-manager.h"

using ::fst::Project;
using ::fst::ProjectType;
using ::fst::RandGen;
using ::fst::RandGenOptions;
using ::fst::StdArc;
using ::fst::StdVectorFst;
using ::fst::SymbolTable;
using ::fst::TokenType;
using ::fst::UniformArcSelector;
using ::rewrite::LatticeToStrings;
using ::thrax::GrmManagerSpec;

ABSL_FLAG(std::string, far, "", "Path to the FAR");
ABSL_FLAG(std::string, rule, "", "Names of the rewrite rule");
ABSL_FLAG(std::string, input_mode, "byte",
          "Either \"byte\", \"utf8\", or the path to "
          "a symbol table for input parsing");
ABSL_FLAG(std::string, output_mode, "byte",
          "Either \"byte\", \"utf8\", or the path to "
          "a symbol table for input parsing");
ABSL_FLAG(int64_t, noutput, 1, "Maximum number of output mappings to generate");

int main(int argc, char** argv) {
  fst::InitOpenFst(argv[0], &argc, &argv, true);

  GrmManagerSpec<StdArc> grm;
  CHECK(grm.LoadArchive(absl::GetFlag(FLAGS_far)));
  std::unique_ptr<SymbolTable> output_syms;
  TokenType type;
  if (absl::GetFlag(FLAGS_output_mode) == "byte") {
    type = TokenType::BYTE;
  } else if (absl::GetFlag(FLAGS_output_mode) == "utf8") {
    type = TokenType::UTF8;
  } else {
    type = TokenType::SYMBOL;
    output_syms.reset(SymbolTable::ReadText(absl::GetFlag(FLAGS_output_mode)));
    if (!output_syms) {
      LOG(FATAL) << "Invalid mode or symbol table path";
    }
  }
  std::unique_ptr<SymbolTable> input_syms;
  if (absl::GetFlag(FLAGS_input_mode) == "byte") {
    type = TokenType::BYTE;
  } else if (absl::GetFlag(FLAGS_input_mode) == "utf8") {
    type = TokenType::UTF8;
  } else {
    type = TokenType::SYMBOL;
    input_syms.reset(SymbolTable::ReadText(absl::GetFlag(FLAGS_input_mode)));
    if (!input_syms) {
      LOG(FATAL) << "Invalid mode or symbol table path";
    }
  }
  if (absl::GetFlag(FLAGS_rule).empty()) {
    LOG(FATAL) << "--rule must be specified";
  }
  const auto* fst = grm.GetFst(absl::GetFlag(FLAGS_rule));
  if (!fst) {
    LOG(FATAL) << "grm.GetFst() must be non nullptr for rule: "
               << absl::GetFlag(FLAGS_rule);
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

  for (int i = 0; i < absl::GetFlag(FLAGS_noutput); ++i) {
    StdVectorFst ofst;
    RandGen(cleaned, &ofst, opts);
    if (ofst.NumStates() == 0) continue;
    StdVectorFst ifst(ofst);
    Project(&ifst, ProjectType::INPUT);
    Project(&ofst, ProjectType::OUTPUT);
    if (!LatticeToStrings(ifst, &istrings, type, input_syms.get())) {
      LOG(FATAL) << "Can't generate strings for input side";
    }
    if (!LatticeToStrings(ofst, &ostrings, type, output_syms.get())) {
      LOG(FATAL) << "Can't generate strings for output side";
    }
  }
  for (size_t i = 0; i < istrings.size(); ++i) {
    std::cout << "****************************************" << std::endl;
    std::cout << istrings[i].first << std::endl
              << ostrings[i].first << std::endl;
    // TODO: Currently there is an issue that RandGen() removes weights, so
    // we'll never actually see these costs.
    if (istrings[i].second != 0) {
      std::cout << " <cost=" << istrings[i].second << ">" << std::endl;
    }
  }

  return 0;
}
