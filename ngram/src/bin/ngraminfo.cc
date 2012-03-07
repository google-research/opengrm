// ngraminfo.cc
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
// Prints out various information about n-gram language modesl.
//

#include <iomanip>
#include <fst/vector-fst.h>
#include <ngram/ngram-model.h>

using namespace std;

namespace ngram {

void PrintNGramInfo(const NGramModel &ngram, ostream &ostrm) {
  const StdExpandedFst &fst = ngram.GetFst();
  vector<size_t> order_ngrams(ngram.HiOrder(), 0);
  size_t ngrams = 0;
  size_t backoffs = 0;
  size_t nfinal = 0;
  for (size_t s = 0; s < fst.NumStates(); ++s) {
    int order = ngram.StateOrder(s);
    if (fst.Final(s) != StdArc::Weight::Zero()) {
      ++nfinal;
      if (order > 0)
	++order_ngrams[order - 1];
    }
    for(ArcIterator<StdExpandedFst> aiter(fst, s);
        !aiter.Done(); aiter.Next()) {
      const StdArc &arc = aiter.Value();
      if (arc.ilabel == 0) {
        ++backoffs;
      } else {
        ++ngrams;
        if (order > 0)
          ++order_ngrams[order - 1];
      }
    }
  }

  ostrm << left << setw(50) << "# of states" << fst.NumStates() << endl;
  ostrm << setw(50) << "# of ngram arcs" << ngrams << endl;
  ostrm << setw(50) << "# of backoff arcs" << backoffs << endl;
  ostrm << setw(50) << "initial state" << fst.Start() << endl;
  ostrm << setw(50) << "unigram state" << ngram.UnigramState() << endl;
  ostrm << setw(50) << "# of final states" <<  nfinal << endl;

  ostrm << setw(50) << "ngram order" << ngram.HiOrder() << endl;
  for (int order = 1; order <= ngram.HiOrder(); ++order) {
    stringstream label;
    label << "# of " << order << "-grams";
    ostrm << setw(50) << label.str() << order_ngrams[order-1] << endl;
  }
  ostrm << setw(50) << "well-formed"
       << (ngram.CheckTopology() ? 'y' : 'n') << endl;
  ostrm << setw(50) << "normalized"
       << (ngram.CheckNormalization() ? 'y' : 'n') << endl;
}

} // namespace ngram


int main(int argc, char **argv) {
  using fst::StdFst;
  using fst::StdMutableFst;
  using fst::StdVectorFst;

  string usage = "Prints out various information about an LM.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst [out.txt]]\n";

  InitFst(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  string ifile = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";

  StdMutableFst *fst = StdMutableFst::Read(ifile, true);
  if (!fst) return 1;

  ostream *ostrm = (argc > 2 && (strcmp(argv[2], "-") != 0)) ?
    new ofstream(argv[2]) : &std::cout;
  if (!(*ostrm))
    LOG(FATAL) << "Can't open for writing";

  ngram::NGramModel ngram(fst);
  ngram::PrintNGramInfo(ngram, *ostrm);

  return 0;
}
