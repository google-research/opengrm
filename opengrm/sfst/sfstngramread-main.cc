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

#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>
#include <memory>
#include <ostream>
#include <string>

#include "openfst/compat/init.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "openfst/lib/arc.h"  // NOLINT(misc-include-cleaner)
#include "openfst/lib/fst.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"  // NOLINT(misc-include-cleaner)
#include "opengrm/sfst/arpa.h"

ABSL_FLAG(std::string, symbols, "", "Symbol table file");

int sfstngramread_main(int argc, char** argv) {
  std::string usage = "Transform ARPA text format to FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.txt [out.fst]]\n";

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

  std::unique_ptr<fst::SymbolTable> syms;
  if (!absl::GetFlag(FLAGS_symbols).empty()) {
    syms.reset(fst::SymbolTable::ReadText(absl::GetFlag(FLAGS_symbols)));
    if (!syms) {
      LOG(ERROR) << argv[0] << ": Error reading symbol table: "
                 << absl::GetFlag(FLAGS_symbols);
      return 1;
    }
  }

  fst::VectorFst<fst::StdArc> fst;
  if (syms) {
    fst.SetInputSymbols(syms.get());
    fst.SetOutputSymbols(syms.get());
  }

  sfst::ReadArpa(istrm, &fst);

  if (!fst.Write(ostrm, fst::FstWriteOptions())) {
    LOG(ERROR) << argv[0] << ": Write failed";
    return 1;
  }

  return 0;
}
