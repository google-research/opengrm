
// Licensed under the Apache License, Version 2.0 (the 'License');
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an 'AS IS' BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2005-2016 Brian Roark and Google, Inc.
// To intersect ngram fst with input fst archive.

#include <memory>
#include <string>

#include <fst/flags.h>
#include <fst/extensions/far/far.h>
#include <fst/fst.h>
#include <ngram/lexicographic-map.h>
#include <ngram/ngram-output.h>

DEFINE_string(bo_arc_type, "phi",
              "One of: \"phi\" (default), \"epsilon\", \"lexicographic\"");

enum BACKOFF_TYPE { PHI, EPS, LEX_EPS };

int main(int argc, char** argv) {
  string usage = "Intersect n-gram model with fst archive.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] ngram.fst [in.far [out.far]]\n";
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc < 2 || argc > 4) {
    ShowUsage();
    return 1;
  }

  BACKOFF_TYPE type;
  if (FLAGS_bo_arc_type == "phi") {
    type = PHI;
  } else if (FLAGS_bo_arc_type == "epsilon") {
    type = EPS;
  } else if (FLAGS_bo_arc_type == "lexicographic") {
    type = LEX_EPS;
  } else {
    NGRAMERROR() << "Unknown backoff arc type: " << FLAGS_bo_arc_type;
    return 1;
  }

  // TODO(rws): This is temporary to avoid issues having to do with
  // symbol table compatibility. At some point we need to sanitize all
  // of that.
  FLAGS_fst_compat_symbols = false;
  fst::FstReadOptions opts;

  string in1_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
  std::unique_ptr<fst::StdVectorFst> lmfst(
      fst::StdVectorFst::Read(in1_name));
  if (!lmfst) return 1;

  ngram::NGramOutput ngram(lmfst.get());
  if (ngram.Error()) {
      NGRAMERROR() << argv[0] << ": Failed to initialize ngram model.";
      return 1;
  }
  std::unique_ptr<ngram::StdLexicographicRescorer> lex_rescorer;
  if (type == LEX_EPS) {
    lex_rescorer.reset(
        new ngram::StdLexicographicRescorer(lmfst.get(), &ngram));
  } else if (type == PHI) {
    ngram.MakePhiMatcherLM(ngram::kSpecialLabel);
  }

  string in2_name = (argc > 2 && strcmp(argv[2], "-") != 0) ? argv[2] : "";
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
  fst::FarType far_type = fst::FAR_STLIST;
  string out_name = (argc > 3 && strcmp(argv[3], "-") != 0) ? argv[3] : "";
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
    cfst.reset(type == LEX_EPS ? lex_rescorer->Rescore(lattice.get())
                               : new fst::StdVectorFst());
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
    if (FLAGS_v > 0)
      std::cerr << "Done:\t" << far_reader->GetKey() << '\n';
  }
  return 0;
}
