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
// Copyright 2009-2013 Brian Roark and Google, Inc.
// Authors: roarkbr@gmail.com  (Brian Roark)
//          allauzen@google.com (Cyril Allauzen)
//          riley@google.com (Michael Riley)
//
// \file
// Prints out various information about n-gram language models.

#include <iomanip>
#include <fst/vector-fst.h>
#include <ngram/ngram-model.h>

using namespace std;

DEFINE_bool(check_consistency, true, "Check model consistency");

namespace ngram {

void PrintNGramInfo(const NGramModel &ngram, ostream &ostrm) {
  const StdFst &fst = ngram.GetFst();
  std::vector<size_t> order_ngrams(ngram.HiOrder(), 0);
  size_t ngrams = 0;
  size_t backoffs = 0;
  size_t nfinal = 0;
  for (size_t s = 0; s < ngram.NumStates(); ++s) {
    int order = ngram.StateOrder(s);
    if (fst.Final(s) != StdArc::Weight::Zero()) {
      ++nfinal;
      if (order > 0)
	++order_ngrams[order - 1];
    }
    for (ArcIterator<StdFst> aiter(fst, s);
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

  ios_base::fmtflags old = ostrm.setf(ios::left);
  ostrm.width(50);
  ostrm << "# of states" << ngram.NumStates() << "\n";
  ostrm.width(50);
  ostrm << "# of ngram arcs" << ngrams << "\n";
  ostrm.width(50);
  ostrm << "# of backoff arcs" << backoffs << "\n";
  ostrm.width(50);
  ostrm << "initial state" << fst.Start() << "\n";
  ostrm.width(50);
  ostrm << "unigram state" << ngram.UnigramState() << "\n";
  ostrm.width(50);
  ostrm << "# of final states" <<  nfinal << "\n";

  ostrm.width(50);
  ostrm << "ngram order" << ngram.HiOrder() << "\n";
  for (int order = 1; order <= ngram.HiOrder(); ++order) {
    stringstream label;
    label << "# of " << order << "-grams";
    ostrm.width(50);
    ostrm << label.str() << order_ngrams[order-1] << "\n";
  }
  ostrm.width(50);
  ostrm << "well-formed"
       << (ngram.CheckTopology() ? 'y' : 'n') << "\n";
  ostrm.width(50);
  ostrm << "normalized"
       << (ngram.CheckNormalization() ? 'y' : 'n') << "\n";
  ostrm.flush();
  ostrm.setf(old);
}

} // namespace ngram


int main(int argc, char **argv) {
  using fst::StdFst;
  using fst::StdMutableFst;
  using fst::StdVectorFst;

  string usage = "Prints out various information about an LM.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst [out.txt]]\n";

  SET_FLAGS(usage.c_str(), &argc, &argv, true);
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

  ngram::NGramModel ngram(*fst, 0, ngram::kNormEps, FLAGS_check_consistency);
  if (FLAGS_check_consistency && !ngram.CheckTopology())
    LOG(FATAL) << "Bad ngram model topology";

  ngram::PrintNGramInfo(ngram, *ostrm);
  if (ostrm != &std::cout)
    delete ostrm;

  return 0;
}
