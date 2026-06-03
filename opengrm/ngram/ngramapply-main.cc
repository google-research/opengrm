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

// Intersects n-gram FST with input FST archive.

#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "absl/log/vlog_is_on.h"
#include "openfst/extensions/far/far.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/determinize.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/rmepsilon.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/ngram/lexicographic-map.h"
#include "opengrm/ngram/ngram-output.h"
#include "opengrm/ngram/util.h"

ABSL_DECLARE_FLAG(std::string, bo_arc_type);

enum BACKOFF_TYPE { PHI, EPS, LEX_EPS };

int ngramapply_main(int argc, char** argv) {
  std::string usage = "Intersects n-gram model with FST archive.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] ngram.fst [in.far [out.far]]\n";
  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  if (argc < 2 || argc > 4) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  BACKOFF_TYPE type;
  if (absl::GetFlag(FLAGS_bo_arc_type) == "phi") {
    type = PHI;
  } else if (absl::GetFlag(FLAGS_bo_arc_type) == "epsilon") {
    type = EPS;
  } else if (absl::GetFlag(FLAGS_bo_arc_type) == "lexicographic") {
    type = LEX_EPS;
  } else {
    NGRAMERROR() << "Unknown backoff arc type: "
                 << absl::GetFlag(FLAGS_bo_arc_type);
    return 1;
  }

  // TODO: This is temporary to avoid issues having to do with
  // symbol table compatibility. At some point we need to sanitize all
  // of that.
  absl::SetFlag(&FLAGS_fst_compat_symbols, false);
  fst::FstReadOptions opts;

  std::string in1_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
  std::unique_ptr<fst::StdMutableFst> lmfst(
      fst::StdMutableFst::Read(in1_name, /*convert=*/true));
  if (!lmfst) return 1;

  ngram::NGramOutput ngram(lmfst.get());
  if (ngram.Error()) {
    NGRAMERROR() << argv[0] << ": Failed to initialize ngram model.";
    return 1;
  }
  std::unique_ptr<ngram::StdLexicographicRescorer> lex_rescorer;
  if (type == LEX_EPS) {
    lex_rescorer =
        std::make_unique<ngram::StdLexicographicRescorer>(lmfst.get(), &ngram);
  } else if (type == PHI) {
    ngram.MakePhiMatcherLM(ngram::kSpecialLabel);
  }

  std::string in2_name = (argc > 2 && strcmp(argv[2], "-") != 0) ? argv[2] : "";
  if (in2_name.empty()) {
    if (in1_name.empty()) {
      NGRAMERROR() << argv[0] << ": Can't use standard i/o for both inputs.";
      return 1;
    }
  }

  std::unique_ptr<fst::FarReader<fst::StdArc>> far_reader(
      fst::FarReader<fst::StdArc>::Open(in2_name));
  if (!far_reader) {
    NGRAMERROR() << "Can't open " << in2_name << " for reading";
    return 1;
  }
  fst::FarType far_type = fst::FarType::STLIST;
  std::string out_name = (argc > 3 && strcmp(argv[3], "-") != 0) ? argv[3] : "";
  std::unique_ptr<fst::FarWriter<fst::StdArc>> far_writer(
      fst::FarWriter<fst::StdArc>::Create(out_name, far_type));
  if (!far_writer) {
    NGRAMERROR() << "Can't open " << out_name << " for writing";
    return 1;
  }
  while (!far_reader->Done()) {
    std::unique_ptr<fst::StdVectorFst> lattice(
        new fst::StdVectorFst(*far_reader->GetFst()));
    std::unique_ptr<fst::StdVectorFst> cfst;
    if (type == LEX_EPS) {
      if (!lex_rescorer) {
        NGRAMERROR() << "`lex_rescorer` not initialized!";
        return 1;
      }
      cfst.reset(lex_rescorer->Rescore(lattice.get()));
    } else {
      cfst.reset(new fst::StdVectorFst());
    }

    if (type != LEX_EPS) {
      if (type == PHI) {
        ngram.FailLMCompose(*lattice, cfst.get(), ngram::kSpecialLabel);
      } else {
        fst::StdVectorFst dfst;
        fst::Compose(*lattice, *lmfst, &dfst);
        fst::RmEpsilon(&dfst);
        fst::Determinize(dfst, cfst.get());
      }
    }
    cfst->SetInputSymbols(lattice->InputSymbols());
    cfst->SetOutputSymbols(lattice->OutputSymbols());
    far_writer->Add(far_reader->GetKey(), *cfst);
    far_reader->Next();
    if (VLOG_IS_ON(1)) std::cerr << "Done:\t" << far_reader->GetKey() << '\n';
  }
  return 0;
}
