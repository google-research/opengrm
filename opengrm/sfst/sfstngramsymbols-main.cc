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

// Derives a symbol table from an input text corpus.
// Adapted for SFst.

#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>

#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "absl/strings/str_split.h"
#include "openfst/lib/symbol-table.h"

ABSL_DECLARE_FLAG(std::string, epsilon_symbol);
ABSL_DECLARE_FLAG(std::string, OOV_symbol);

int sfstngramsymbols_main(int argc, char** argv) {
  std::string usage = "Derives a symbol table from a corpus.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.txt [out.txt]]\n";
  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    std::cerr << usage << "\n";
    return 1;
  }

  std::ifstream ifstrm;
  if (argc > 1 && (strcmp(argv[1], "-") != 0)) {
    ifstrm.open(argv[1]);
    if (!ifstrm) {
      LOG(ERROR) << argv[0] << ": Open failed: " << argv[1];
      return 1;
    }
  }
  std::istream& istrm = ifstrm.is_open() ? ifstrm : std::cin;

  std::ofstream ofstrm;
  if (argc > 2 && (strcmp(argv[2], "-") != 0)) {
    ofstrm.open(argv[2]);
    if (!ofstrm) {
      LOG(ERROR) << argv[0] << ": Open failed: " << argv[2];
      return 1;
    }
  }
  std::ostream& ostrm = ofstrm.is_open() ? ofstrm : std::cout;

  fst::SymbolTable syms("SFstNGramSymbols");

  // Epsilon symbol must be added first (index 0).
  syms.AddSymbol(absl::GetFlag(FLAGS_epsilon_symbol));
  for (std::string str; std::getline(istrm, str);) {
    for (absl::string_view token :
         absl::StrSplit(str, absl::ByAnyChar(" \t"), absl::SkipEmpty())) {
      syms.AddSymbol(token);
    }
  }
  std::string oov = absl::GetFlag(FLAGS_OOV_symbol);
  if (!oov.empty()) syms.AddSymbol(oov);

  return syms.WriteText(ostrm) ? 0 : 1;
}
