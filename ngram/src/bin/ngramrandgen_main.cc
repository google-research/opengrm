
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
// Copyright 2005-2016 Brian Roark and Google, Inc.
// Generates random sentences from an LM or more generally paths through any
// FST where epsilons are treated as failure transitions.

#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <vector>

#include <fst/extensions/far/far.h>
#include <fst/fst.h>
#include <fst/randgen.h>
#include <fst/rmepsilon.h>
#include <fst/shortest-path.h>
#include <fst/vector-fst.h>
#include <ngram/ngram-randgen.h>

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
  std::stringstream ss;
  ss << value;
  string extension;
  ss >> extension;
  if (key) {
    for (int i = extension.length(); i < far_len; i++) (*key) += "0";
    (*key) += extension;
  }
  return extension.length();
}

// Write given fst into open far_writer
void FarWriteFst(fst::FarWriter<fst::StdArc> *far_writer,
                 fst::StdFst *fst, int *far_incr, int far_len) {
  string key = "FST";
  CalcExtLen((*far_incr)++, &key, far_len);
  far_writer->Add(key, *fst);
}

// from the given state to a final state, create linear fst for string
void CreateStringFstFromPath(std::vector<int> *labels,
                             fst::StdVectorFst *nfst) {
  fst::StdArc::StateId ost = 0, dst = 1;
  nfst->AddState();
  nfst->SetStart(ost);
  for (int i = 0; i < labels->size(); i++) {
    nfst->AddState();
    nfst->AddArc(
        ost++, fst::StdArc((*labels)[i], (*labels)[i],
                               fst::StdArc::Weight::One().Value(), dst++));
  }
  nfst->SetFinal(ost, fst::StdArc::Weight::One().Value());
}

// Given an fst (tree) of strings, create far of string fsts
int WritePathsToFar(fst::StdFst *fst, fst::StdArc::StateId st,
                    std::vector<int> *labels,
                    fst::FarWriter<fst::StdArc> *far_writer,
                    int far_cnt, int far_len, int remove_epsilons) {
  for (fst::ArcIterator<fst::StdFst> aiter(*fst, st); !aiter.Done();
       aiter.Next()) {
    fst::StdArc arc = aiter.Value();
    if (!remove_epsilons || arc.ilabel != 0) labels->push_back(arc.ilabel);
    far_cnt = WritePathsToFar(fst, arc.nextstate, labels, far_writer, far_cnt,
                              far_len, remove_epsilons);
    if (!remove_epsilons || arc.ilabel != 0) labels->pop_back();
  }
  if (fst->Final(st) != fst::StdArc::Weight::Zero()) {
    fst::StdVectorFst nfst;
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
  string usage = "Generates random sentences from an LM.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst [out.far]]\n";
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  VLOG(1) << argv[0] << ": Seed = " << FLAGS_seed;

  string ifile = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  string ofile = (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "";

  fst::StdFst *ifst = fst::StdFst::Read(ifile);
  if (!ifst) return 1;

  std::unique_ptr<fst::FarWriter<fst::StdArc>> far_writer(
      fst::FarWriter<fst::StdArc>::Create(
          ofile, fst::FAR_STLIST));  // type change for fst
  if (!far_writer) {
    LOG(ERROR) << "Can't open " << (ofile.empty() ? "stdout" : ofile)
               << " for writing";
    return 1;
  }
  fst::StdVectorFst ofst;

  ngram::NGramArcSelector<fst::StdArc> selector(FLAGS_seed);
  fst::RandGenOptions<ngram::NGramArcSelector<fst::StdArc>> opts(
      selector, FLAGS_max_length, FLAGS_max_sents, FLAGS_weighted,
      FLAGS_remove_total_weight);
  fst::RandGen(*ifst, &ofst, opts);
  ofst.SetInputSymbols(ifst->InputSymbols());  // takes model symbol tables
  ofst.SetOutputSymbols(ifst->OutputSymbols());
  if (FLAGS_weighted) {
    int far_incr = 1, far_len = 1;
    FarWriteFst(far_writer.get(), &ofst, &far_incr, far_len);
  } else {
    int far_cnt = 1, far_len = CalcExtLen(FLAGS_max_sents, 0, 0);
    std::vector<int> labels;
    WritePathsToFar(&ofst, ofst.Start(), &labels, far_writer.get(), far_cnt,
                    far_len, FLAGS_remove_epsilon);
  }

  return 0;
}
