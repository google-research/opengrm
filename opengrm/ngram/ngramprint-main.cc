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

// Prints a given n-gram model to various kinds of textual formats.

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/symbol-table.h"
#include "opengrm/ngram/ngram-output.h"

ABSL_DECLARE_FLAG(bool, ARPA);
ABSL_DECLARE_FLAG(bool, backoff);
ABSL_DECLARE_FLAG(bool, backoff_inline);
ABSL_DECLARE_FLAG(bool, negativelogs);
ABSL_DECLARE_FLAG(bool, integers);
ABSL_DECLARE_FLAG(int64_t, backoff_label);
ABSL_DECLARE_FLAG(bool, check_consistency);
ABSL_DECLARE_FLAG(std::string, context_pattern);
ABSL_DECLARE_FLAG(bool, include_all_suffixes);
ABSL_DECLARE_FLAG(std::string, symbols);

int ngramprint_main(int argc, char** argv) {
  std::string usage = "Print n-gram counts and models.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst [out.txt]]\n";
  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  std::string out_name =
      (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "stdout";

  std::unique_ptr<fst::StdMutableFst> fst(
      fst::StdMutableFst::Read(in_name, true));
  if (!fst) return 1;

  if (!absl::GetFlag(FLAGS_symbols).empty()) {
    std::unique_ptr<fst::SymbolTable> syms(
        fst::SymbolTable::ReadText(absl::GetFlag(FLAGS_symbols)));
    if (!syms) return 1;
    fst->SetInputSymbols(syms.get());
    fst->SetOutputSymbols(syms.get());
  }

  std::ofstream ofstrm;
  if (argc > 2 && (strcmp(argv[2], "-") != 0)) {
    ofstrm.open(argv[2]);
    if (!ofstrm) {
      LOG(ERROR) << argv[0] << ": Open failed, file = " << argv[2];
      return 1;
    }
  }
  std::ostream& ostrm = ofstrm.is_open() ? ofstrm : std::cout;

  if (absl::GetFlag(FLAGS_ARPA) &&
      (absl::GetFlag(FLAGS_negativelogs) || absl::GetFlag(FLAGS_integers))) {
    LOG(ERROR) << "Printing in ARPA format writes weights as log_{10} "
               << "probabilities. --ARPA cannot be used with "
               << "the --negativelogs and --integers flags.";
    return 1;
  }

  ngram::NGramOutput ngram(fst.get(), ostrm, absl::GetFlag(FLAGS_backoff_label),
                           absl::GetFlag(FLAGS_check_consistency),
                           absl::GetFlag(FLAGS_context_pattern),
                           absl::GetFlag(FLAGS_include_all_suffixes));

  // Parse --backoff and --backoff_inline flags, where --backoff takes precedent
  ngram::NGramOutput::ShowBackoff show_backoff =
      ngram::NGramOutput::ShowBackoff::NONE;
  if (absl::GetFlag(FLAGS_backoff)) {
    show_backoff = ngram::NGramOutput::ShowBackoff::EPSILON;
    if (absl::GetFlag(FLAGS_backoff_inline))
      show_backoff = ngram::NGramOutput::ShowBackoff::INLINE;
  }

  ngram.ShowNGramModel(show_backoff, absl::GetFlag(FLAGS_negativelogs),
                       absl::GetFlag(FLAGS_integers),
                       absl::GetFlag(FLAGS_ARPA));
  return 0;
}
