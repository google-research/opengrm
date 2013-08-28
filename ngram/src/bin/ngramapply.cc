// ngramapply.cc
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
// Copyright 2011-2013 Richard Sproat, Brian Roark and Google, Inc.
// Author: rws@xoba.com (Richard Sproat)
//         roarkbr@gmail.com  (Brian Roark)
//         allauzen@google.com (Cyril Allauzen)
//         riley@google.com (Michael Riley)
//
// \file
// To intersect ngram fst with input fst archive

#include <fst/extensions/far/far.h>
#include <fst/fst.h>
#include <ngram/lexicographic-map.h>
#include <ngram/ngram-output.h>
#include <string>

using namespace fst;
using namespace ngram;
using std::string;

DEFINE_string(bo_arc_type, "phi",
	      "One of: \"phi\" (default), \"epsilon\", \"lexicographic\"");

enum BACKOFF_TYPE { PHI, EPS, LEX_EPS };

inline void AddToFar(MutableFst<StdArc>* olattice,
		     MutableFst<StdArc>* ilattice,
		     FarReader<StdArc>* far_reader,
		     FarWriter<StdArc>* far_writer) {
  olattice->SetInputSymbols(ilattice->InputSymbols());
  olattice->SetOutputSymbols(ilattice->OutputSymbols());
  far_writer->Add(far_reader->GetKey(), *olattice);
}

int main(int argc, char **argv) {
  string usage = "Intersect n-gram model with fst archive.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] ngram.fst [in.far [out.far]]\n";
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc < 2 || argc > 4) {
    ShowUsage();
    return 1;
  }

  BACKOFF_TYPE type;
  if (FLAGS_bo_arc_type == "phi")
    type = PHI;
  else if (FLAGS_bo_arc_type == "epsilon")
    type = EPS;
  else if (FLAGS_bo_arc_type == "lexicographic")
    type = LEX_EPS;
  else
    LOG(FATAL) << "Unknown backoff arc type: " << FLAGS_bo_arc_type;

  // TODO(rws): This is temporary to avoid issues having to do with
  // symbol table compatibility. At some point we need to sanitize all
  // of that.
  FLAGS_fst_compat_symbols = false;
  FstReadOptions opts;

  string in1_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
  StdMutableFst *lmfst = StdMutableFst::Read(in1_name, true);
  if (!lmfst) return 1;

  NGramOutput ngram(lmfst);
  StdLexicographicRescorer* lex_rescorer = NULL;
  if (type == LEX_EPS) {
    lex_rescorer = new StdLexicographicRescorer(lmfst, &ngram);
  } else if (type == PHI) {
    ngram.MakePhiMatcherLM(kSpecialLabel);
  }

  string in2_name = (argc > 2 && strcmp(argv[2], "-") != 0) ? argv[2] : "";
  if (in2_name.empty()) {
    if (in1_name.empty()) {
      LOG(ERROR) << argv[0]
		 << ": Can't use standard i/o for both inputs.";
      return 1;
    }
  }

  FarReader<StdArc>* far_reader = FarReader<StdArc>::Open(in2_name);
  if (!far_reader) {
    LOG(ERROR) << "Can't open " << in2_name << " for reading";
    return 1;
  }
  FarType far_type = FAR_STLIST;
  string out_name = (argc > 3 && strcmp(argv[3], "-") != 0) ? argv[3] : "";
  FarWriter<StdArc>* far_writer =
    FarWriter<StdArc>::Create(out_name, far_type);
  if (!far_writer) {
    LOG(ERROR) << "Can't open " << out_name << " for writing";
    return 1;
  }
  while (!far_reader->Done()) {
    StdMutableFst *lattice = MutableFstConvert(far_reader->GetFst().Copy());
    if (type == LEX_EPS) {
      StdVectorFst* cfst = lex_rescorer->Rescore(lattice);
      AddToFar(cfst, lattice, far_reader, far_writer);
    } else if (type == PHI) {
      StdVectorFst cfst;
      ngram.FailLMCompose(*lattice, &cfst, kSpecialLabel);
      AddToFar(&cfst, lattice, far_reader, far_writer);
    } else {
      StdVectorFst cfst;
      Compose(*lattice, *lmfst, &cfst);
      RmEpsilon(&cfst);
      StdVectorFst dfst;
      Determinize(cfst, &dfst);
      AddToFar(&dfst, lattice, far_reader, far_writer);
    }
    delete lattice;
    far_reader->Next();
    if (FLAGS_v > 0)
      cerr << "Done:\t" << far_reader->GetKey() << '\n';
  }
  delete lmfst;
  delete far_reader;
  delete lex_rescorer;
  delete far_writer;
  return 0;
}
