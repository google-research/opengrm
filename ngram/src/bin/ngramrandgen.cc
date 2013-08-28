// ngramrandgen.cc
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
// Copyright 2009-2013 Brian Roark and Google, Inc.
// Authors: roarkbr@gmail.com  (Brian Roark)
//          allauzen@google.com (Cyril Allauzen)
//          riley@google.com (Michael Riley)
//
// \file
// Generates random sentences from an LM or more generally paths through any
// FST where epsilons are treated as failure transitions.

#include <string>
#include <fst/fst.h>
#include <fst/vector-fst.h>
#include <fst/shortest-path.h>
#include <fst/rmepsilon.h>
#include <fst/randgen.h>
#include <fst/extensions/far/far.h>

#include <ngram/ngram-randgen.h>

using namespace fst;
typedef StdArc::StateId StateId;

DEFINE_int32(max_length, INT_MAX, "Maximum sentence length");
DEFINE_int64(max_sents, 1, "Maximum number of sentences to produce");
DEFINE_int32(seed, time(0) + getpid(), "Randomization seed");
DEFINE_bool(remove_epsilon, false, "Remove epsilons from generated strings");
DEFINE_bool(weighted, false,
            "Output tree weighted by sentence count vs. unweighted sentences");
DEFINE_bool(remove_total_weight, false,
            "Remove total weight when output weighted");

// create FST label for far, with sufficient padding for the number
int CalcExtLen(int value, string *key, int far_len) {
  stringstream ss;
  ss << value;
  string extension;
  ss >> extension;
  if (key) {
    for (int i = extension.length(); i < far_len; i++)
      (*key) += "0";
    (*key) += extension;
  }
  return extension.length();
}

// Write given fst into open far_writer
void FarWriteFst(FarWriter<StdArc> *far_writer, StdFst *fst,
		 int *far_incr, int far_len) {
  string key = "FST";
  CalcExtLen((*far_incr)++, &key, far_len);
  far_writer->Add(key, *fst);
}

// from the given state to a final state, create linear fst for string
void CreateStringFstFromPath(vector<int> *labels, StdVectorFst *nfst) {
  StateId ost = 0, dst = 1;
  nfst->AddState();
  nfst->SetStart(ost);
  for (int i = 0; i < labels->size(); i++) {
    nfst->AddState();
    nfst->AddArc(ost++, StdArc((*labels)[i], (*labels)[i],
			       StdArc::Weight::One().Value(), dst++));
  }
  nfst->SetFinal(ost, StdArc::Weight::One().Value());
}

// Given an fst (tree) of strings, create far of string fsts
int WritePathsToFar(StdFst *fst, StateId st, vector<int> *labels,
		    FarWriter<StdArc> *far_writer, int far_cnt,
		    int far_len, int remove_epsilons) {
  for (ArcIterator< StdFst > aiter(*fst, st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (!remove_epsilons || arc.ilabel != 0)
      labels->push_back(arc.ilabel);
    far_cnt = WritePathsToFar(fst, arc.nextstate, labels, far_writer,
			      far_cnt, far_len, remove_epsilons);
    if (!remove_epsilons || arc.ilabel != 0)
      labels->pop_back();
  }
  if (fst->Final(st) != StdArc::Weight::Zero()) {
    StdVectorFst nfst;
    if (far_cnt == 1) {  // assigns symbol table to first fst
      nfst.SetInputSymbols(fst->InputSymbols());
      nfst.SetOutputSymbols(fst->OutputSymbols());
    }
    CreateStringFstFromPath(labels, &nfst);
    FarWriteFst(far_writer, &nfst, &far_cnt, far_len);
  }
  return far_cnt;
}

int main(int argc, char **argv) {
  using fst::StdFst;
  using fst::StdVectorFst;
  using fst::RandGenOptions;
  using fst::RandGen;

  using fst::StdArc;
  using ngram::NGramArcSelector;

  string usage = "Generates random sentences from an LM.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst [out.far]]\n";
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  VLOG(1) << argv[0] << ": Seed = " << FLAGS_seed;

  string ifile = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  string ofile =  (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "";

  StdFst *ifst = StdFst::Read(ifile);
  if (!ifst) return 1;

  FarWriter<StdArc>* far_writer =
    FarWriter<StdArc>::Create(ofile, FAR_STLIST);  // type change for fst
  if (!far_writer) {
    LOG(ERROR) << "Can't open " << (ofile.empty() ? "stdout" : ofile)
	       << " for writing";
    return 1;
  }
  StdVectorFst ofst;

  NGramArcSelector<StdArc> selector(FLAGS_seed);
  RandGenOptions< NGramArcSelector<StdArc> >
      opts(selector, FLAGS_max_length, FLAGS_max_sents, FLAGS_weighted,
           FLAGS_remove_total_weight);
  RandGen(*ifst, &ofst, opts);
  ofst.SetInputSymbols(ifst->InputSymbols());  // takes model symbol tables
  ofst.SetOutputSymbols(ifst->OutputSymbols());
  if (FLAGS_weighted) {
    int far_incr = 1, far_len = 1;
    FarWriteFst(far_writer, &ofst, &far_incr, far_len);
  } else {
    int far_cnt = 1, far_len = CalcExtLen(FLAGS_max_sents, 0, 0);
    vector<int> labels;
    WritePathsToFar(&ofst, ofst.Start(), &labels, far_writer,
		    far_cnt, far_len, FLAGS_remove_epsilon);
  }
  delete far_writer;

  return 0;
}
