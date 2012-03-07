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
// Copyright 2009-2011 Brian Roark and Google, Inc.
// Authors: roarkbr@gmail.com  (Brian Roark)
//          allauzen@google.com (Cyril Allauzen)
//          riley@google.com (Michael Riley)
//
// \file
// TODO: file description & doc.

#include <fst/fst.h>
#include <fst/extensions/far/far.h>
#include <fst/map.h>
#include <fst/arcsort.h>
#include <fst/vector-fst.h>
#include <ngram/ngram-count.h>
#include <string>

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


using namespace ngram;
using namespace fst;
using namespace std;

DEFINE_int64(order, 3, "Set maximal order of ngrams to be counted");
DEFINE_bool(epsilon_as_backoff, false,
            "Treat epsilon in the input Fsts as backoff");

int main(int argc, char **argv) {
  string usage = "Count ngram from input file.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.far [out.fst]]\n";
  InitFst(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  NGramCounter<Log64Weight> ngram_counter(FLAGS_order,
                                          FLAGS_epsilon_as_backoff);

  FstReadOptions opts;
  FarReader<StdArc>* far_reader;
  far_reader = FarReader<StdArc>::Open(
     (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : string());
  if (!far_reader) {
    LOG(ERROR) << "Can't open "
               << (strlen(argv[1]) == 0 || strcmp(argv[1], "-") == 0 ?
		   "stdin" : argv[1])
               << " for reading";
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

  if (!lfst) {
    LOG(ERROR) << "None of the input FSTs had a symbol table";
    return 1;
  }

  VectorFst<StdArc> fst;
  ngram_counter.GetFst(&fst);
  ArcSort(&fst, StdILabelCompare());
  fst.SetInputSymbols(lfst->InputSymbols());
  fst.SetOutputSymbols(lfst->InputSymbols());
  fst.Write((argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "");

  return 0;
}
