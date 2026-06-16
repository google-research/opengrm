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

// Counts n-grams from an input fst archive (FAR) file and builds SFST topology.

#include <cstdint>  // NOLINT(misc-include-cleaner)
#include <cstring>
#include <iostream>
#include <memory>  // NOLINT(misc-include-cleaner)
#include <string>

#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "openfst/extensions/far/far-reader.h"  // NOLINT(misc-include-cleaner)
#include "openfst/extensions/far/far.h"  // NOLINT(misc-include-cleaner)
#include "openfst/lib/arc.h"  // NOLINT(misc-include-cleaner)
#include "openfst/lib/float-weight.h"  // NOLINT(misc-include-cleaner)
#include "openfst/lib/fst.h"  // NOLINT(misc-include-cleaner)
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"  // NOLINT(misc-include-cleaner)
#include "opengrm/sfst/ngram-count.h"  // NOLINT(misc-include-cleaner)

ABSL_DECLARE_FLAG(int64_t, order);
ABSL_DECLARE_FLAG(int64_t, phi_label);
ABSL_DECLARE_FLAG(bool, epsilon_as_backoff);
ABSL_DECLARE_FLAG(bool, require_symbols);

int sfstngramcount_main(int argc, char** argv) {
  std::string usage = "Count n-grams from input FAR file to build SFST.\n\n";
  usage += "  Usage: ";
  usage += argv[0];
  usage += " [in.far [out.fst]]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    std::cerr << usage << "\n";
    return 1;
  }

  std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  std::string out_name = argc > 2 ? argv[2] : "";

  std::unique_ptr<fst::FarReader<fst::StdArc>>
      far_reader(  // NOLINT(misc-include-cleaner)
          fst::FarReader<fst::StdArc>::Open(in_name));
  if (!far_reader) {
    LOG(ERROR) << argv[0] << ": Open failed: " << in_name;
    return 1;
  }

  int64_t order = absl::GetFlag(FLAGS_order);
  if (order < 1) {
    LOG(ERROR) << argv[0] << ": order must be positive";
    return 1;
  }

  sfst::NGramCounter<fst::Log64Weight> counter(
      order, absl::GetFlag(FLAGS_epsilon_as_backoff));
  fst::SymbolTable syms;
  bool first_fst = true;
  while (!far_reader->Done()) {
    const fst::StdFst* fst = far_reader->GetFst();
    if (!counter.Count(*fst)) {
      LOG(ERROR) << argv[0] << ": failed to count FST";
      return 1;
    }
    if (first_fst && fst->InputSymbols()) {
      syms = *fst->InputSymbols();
      first_fst = false;
    }
    far_reader->Next();
  }
  if (absl::GetFlag(FLAGS_require_symbols) && syms.NumSymbols() == 0) {
    LOG(ERROR) << argv[0] << ": None of the input FSTs had a symbol table";
    return 1;
  }
  if (counter.Error()) {
    LOG(ERROR) << argv[0] << ": error in counting";
    return 1;
  }

  fst::StdVectorFst out_fst;
  counter.GetFst(&out_fst, absl::GetFlag(FLAGS_phi_label));
  if (syms.NumSymbols() > 0) {
    out_fst.SetInputSymbols(&syms);
    out_fst.SetOutputSymbols(&syms);
  }
  if (!out_fst.Write(out_name)) {
    LOG(ERROR) << argv[0] << ": failed to write output FST";
    return 1;
  }

  return 0;
}
