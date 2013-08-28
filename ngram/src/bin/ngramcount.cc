// ngramcount.cc
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
// Counts n-grams from an input fst archive (FAR) file

#include <fst/fst.h>
#include <fst/extensions/far/far.h>
#include <fst/map.h>
#include <fst/arcsort.h>
#include <fst/vector-fst.h>
#include <ngram/ngram-count.h>
#include <ngram/ngram-count-of-counts.h>
#include <ngram/ngram-model.h>
#include <string>

using namespace ngram;
using namespace fst;
using namespace std;

DEFINE_string(method, "counts", "One of: \"counts\", \"count_of_counts\"");
DEFINE_int64(order, 3, "Set maximal order of ngrams to be counted");
DEFINE_bool(require_symbols, true, "Require symbol tables? (default: yes)");
DEFINE_bool(round_to_int, false, "Round all counts to integers");

// For counting:
DEFINE_bool(epsilon_as_backoff, false,
            "Treat epsilon in the input Fsts as backoff");

// For count-of-counting:
DEFINE_string(context_pattern, "", "Pattern of contexts to count");

namespace fst {

typedef LogWeightTpl<double> Log64Weight;
typedef ArcTpl<Log64Weight> Log64Arc;

template <class Arc>
struct ToLog64Mapper {
  typedef Arc FromArc;
  typedef Log64Arc ToArc;

  ToArc operator()(const FromArc &arc) const {
    return ToArc(arc.ilabel,
                 arc.olabel,
                 arc.weight.Value(),
                 arc.nextstate);
  }

  MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }
  MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS; }
  MapSymbolsAction OutputSymbolsAction() const { return MAP_COPY_SYMBOLS;}
  uint64 Properties(uint64 props) const { return props; }
};

}  // namespace fst

// Rounds -log count to values corresponding to the rounded integer count
// Reduces small floating point precision issues when dealing with int counts
// Primarily for testing that methods for deriving the same model are identical
void RoundCountsToInt(StdMutableFst *fst) {
  for (size_t s = 0; s < fst->NumStates(); ++s) {
    for (MutableArcIterator<StdMutableFst> aiter(fst, s);
	 !aiter.Done();
	 aiter.Next()) {
      StdArc arc = aiter.Value();
      int weight = round(exp(-arc.weight.Value()));
      arc.weight = -log(weight);
      aiter.SetValue(arc);
    }
    if (fst->Final(s) != StdArc::Weight::Zero()) {
      int weight = round(exp(-fst->Final(s).Value()));
      fst->SetFinal(s, -log(weight));
    }
  }
}

// Compute counts
int GetNGramCounts(const string &in_name, const string &out_name) {
  NGramCounter<Log64Weight> ngram_counter(FLAGS_order,
                                          FLAGS_epsilon_as_backoff);

  FstReadOptions opts;
  FarReader<StdArc>* far_reader;
  far_reader = FarReader<StdArc>::Open(in_name);
  if (!far_reader) {
    LOG(ERROR) << "ngramcount: open of FST archive failed: " << in_name;
    return 1;
  }

  int fstnumber = 1;
  const Fst<StdArc> *ifst = 0, *lfst = 0;
  while (!far_reader->Done()) {
    if (ifst)
      delete ifst;
    ifst = far_reader->GetFst().Copy();

    VLOG(1) << opts.source << "#" << fstnumber;
    if (!ifst) {
      LOG(ERROR) << "ngramcount: unable to read fst #" << fstnumber;
      return 1;
    }

    bool counted = false;
    if (ifst->Properties(kString | kUnweighted, true)) {
        counted = ngram_counter.Count(*ifst);
    } else {
      VectorFst<Log64Arc> log_ifst;
      Map(*ifst, &log_ifst, ToLog64Mapper<StdArc>());
      counted = ngram_counter.Count(&log_ifst);
    }
    if (!counted)
      LOG(ERROR) << "ngramcount: fst #" << fstnumber << " skipped";

    if (ifst->InputSymbols() != 0) {  // retain for symbol table
      if (lfst)
	delete lfst;  // delete previously observed symbol table
      lfst = ifst;
      ifst = 0;
    }
    far_reader->Next();
    ++fstnumber;
  }
  delete far_reader;

  if (FLAGS_require_symbols && !lfst) {
    LOG(ERROR) << "None of the input FSTs had a symbol table";
    return 1;
  }

  VectorFst<StdArc> fst;
  ngram_counter.GetFst(&fst);
  ArcSort(&fst, StdILabelCompare());
  if (lfst) {
    fst.SetInputSymbols(lfst->InputSymbols());
    fst.SetOutputSymbols(lfst->InputSymbols());
  }
  if (FLAGS_round_to_int)
    RoundCountsToInt(&fst);
  fst.Write(out_name);

  return 0;
}

// Compute count-of-counts
int GetNGramCountOfCounts(const string &in_name, const string &out_name) {
  StdFst *fst = StdFst::Read(in_name);
  if (!fst) return 1;
  NGramModel ngram(*fst, 0, kNormEps, !FLAGS_context_pattern.empty());
  int order = ngram.HiOrder() > FLAGS_order ? ngram.HiOrder() : FLAGS_order;
  NGramCountOfCounts count_of_counts(FLAGS_context_pattern, order);
  count_of_counts.CalculateCounts(ngram);
  StdVectorFst count_of_counts_fst;
  count_of_counts.GetFst(&count_of_counts_fst);
  count_of_counts_fst.Write(out_name);

  return 0;
}

int main(int argc, char **argv) {
  string usage = "Count ngram from input file.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.far [out.fst]]\n";
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  string in_name = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  string out_name = (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "";

  if (FLAGS_method == "counts") {
    return GetNGramCounts(in_name, out_name);
  } else if (FLAGS_method == "count_of_counts") {
    return GetNGramCountOfCounts(in_name, out_name);
  } else {
    LOG(ERROR) << argv[0] << ": bad counting method: " << FLAGS_method;
    return 1;
  }
}
