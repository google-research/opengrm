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

// Prints out various information about n-gram language models.

#include <cstddef>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/util.h"
#include "opengrm/ngram/ngram-model.h"
#include "opengrm/ngram/util.h"

ABSL_DECLARE_FLAG(bool, check_consistency);
ABSL_DECLARE_FLAG(double, norm_eps);

namespace ngram {

void PrintNGramInfo(const NGramModel<fst::StdArc>& ngram, std::ostream& ostrm) {
  const fst::StdFst& fst = ngram.GetFst();
  std::vector<size_t> order_ngrams(ngram.HiOrder(), 0);
  size_t ngrams = 0;
  size_t backoffs = 0;
  size_t nfinal = 0;
  for (size_t s = 0; s < ngram.NumStates(); ++s) {
    int order = ngram.StateOrder(s);
    if (fst.Final(s) != fst::StdArc::Weight::Zero()) {
      ++nfinal;
      if (order > 0) ++order_ngrams[order - 1];
    }
    for (fst::ArcIterator<fst::StdFst> aiter(fst, s); !aiter.Done();
         aiter.Next()) {
      const fst::StdArc& arc = aiter.Value();
      if (arc.ilabel == 0) {
        ++backoffs;
      } else {
        ++ngrams;
        if (order > 0) ++order_ngrams[order - 1];
      }
    }
  }
  std::ios_base::fmtflags old = ostrm.setf(std::ios::left);
  fst::PrintField(ostrm, "# of states", ngram.NumStates());
  fst::PrintField(ostrm, "# of ngram arcs", ngrams);
  fst::PrintField(ostrm, "# of backoff arcs", backoffs);
  fst::PrintField(ostrm, "initial state", fst.Start());
  fst::PrintField(ostrm, "unigram state", ngram.UnigramState());
  fst::PrintField(ostrm, "# of final states", nfinal);
  fst::PrintField(ostrm, "ngram order", ngram.HiOrder());
  for (int order = 1; order <= ngram.HiOrder(); ++order) {
    fst::PrintField(ostrm, absl::StrCat("# of ", order, "-grams"),
                    order_ngrams[order - 1]);
  }
  fst::PrintField(ostrm, "well-formed", (ngram.CheckTopology() ? 'y' : 'n'));
  fst::PrintField(ostrm, "normalized",
                  (ngram.CheckNormalization() ? 'y' : 'n'));
  ostrm.setf(old);
}

}  // namespace ngram

int ngraminfo_main(int argc, char** argv) {
  std::string usage =
      "Prints out various information about an LM.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst [out.txt]]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  std::string ifile = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";

  std::unique_ptr<fst::StdMutableFst> fst(
      fst::StdMutableFst::Read(ifile, true));
  if (!fst) return 1;

  std::ofstream ofstrm;
  if (argc > 2 && (strcmp(argv[2], "-") != 0)) {
    ofstrm.open(argv[2]);
    if (!ofstrm) {
      LOG(ERROR) << argv[0] << ": Open failed, file = " << argv[2];
      return 1;
    }
  }
  std::ostream& ostrm = ofstrm.is_open() ? ofstrm : std::cout;
  ngram::NGramModel<fst::StdArc> ngram(*fst, 0, absl::GetFlag(FLAGS_norm_eps),
                                       absl::GetFlag(FLAGS_check_consistency));
  if (absl::GetFlag(FLAGS_check_consistency) && !ngram.CheckTopology()) {
    NGRAMERROR() << "Bad ngram model topology";
    return 1;
  }
  ngram::PrintNGramInfo(ngram, ostrm);

  return 0;
}
