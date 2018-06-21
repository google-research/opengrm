
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
// Copyright 2018 Google, Inc.
// sfstperplexity.cc
//
// Computes the perplexity for a stochastic FST. The SFST must be
// normalized. Each evaulated FSTs must be a single path
// (topologically sorted).

#include <stddef.h>
#include <string.h>
#include <iostream>
#include <memory>
#include <string>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/extensions/far/far.h>
#include <fst/fst-decl.h>
#include <fst/fst.h>
#include <fst/shortest-distance.h>
#include <sfst/perplexity.h>

DEFINE_int64(phi_label, fst::kNoLabel,
             "Specifies failure label (default: none)");
DEFINE_int64(unknown_label, fst::kNoLabel,
             "Unknown word label (determines OOV handling)");
DEFINE_int64(unknown_class_size, 10000,
             "Number of members of unknown (OOV) class");
DEFINE_bool(detailed, false, "Compute perplexity per sentence");
DEFINE_double(delta, fst::kDelta, "Comparison delta");
DEFINE_double(shortest_delta, fst::kShortestDelta,
              "Delta for computing shortest distance");

int main(int argc, char **argv) {
  using fst::FarReader;
  using fst::StdArc;
  using fst::StdFst;
  using sfst::Perplexity;

  string usage = "Computes perplexity for an SFST.\n\n  Usage: ";
  usage += argv[0];
  usage += " in.fst [in.far]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc < 2 || argc > 4) {
    ShowUsage();
    return 1;
  }

  string far_name = (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "";

  StdFst *sfst = StdFst::Read(argv[1]);
  if (!sfst) return 1;

  Perplexity<StdArc> perp(*sfst, FLAGS_phi_label, FLAGS_unknown_label,
                          FLAGS_unknown_class_size, FLAGS_delta,
                          FLAGS_shortest_delta);
  std::unique_ptr<FarReader<StdArc>> far_reader(
      FarReader<StdArc>::Open(far_name));

  std::cout.setf(std::ios::left);
  size_t nsent = 0;
  for (++nsent; !far_reader->Done(); ++nsent, far_reader->Next()) {
    if (!perp.Apply(*far_reader->GetFst())) return 1;
    if (FLAGS_detailed) {
      std::cout.width(50);
      std::cout << "sentence" << nsent << std::endl;
      if (perp.NumSkipped() < 1) {
        std::cout.width(50);
        std::cout << "perplexity" << perp.GetPerplexity() << std::endl;
        std::cout.width(50);
        std::cout << "# of words" << perp.NumWords() << std::endl;
        std::cout.width(50);
        std::cout << "# of OOVs" << perp.NumOOVs() << std::endl;
      }
      perp.Reset();
    }
  }

  if (!FLAGS_detailed) {
    std::cout.width(50);
    std::cout << "# of sentences" << perp.NumSentences() << std::endl;
    if (perp.NumSkipped() > 0) {
      std::cout.width(50);
      std::cout << "# of skipped sentences" << perp.NumSkipped() << std::endl;
    }
    if (perp.NumSkipped() < perp.NumSentences()) {
      std::cout.width(50);
      std::cout << "cross entropy" << perp.GetCrossEntropy() << std::endl;
      std::cout.width(50);
      std::cout << "perplexity" << perp.GetPerplexity() << std::endl;
      std::cout.width(50);
      std::cout << "# of words" << perp.NumWords() << std::endl;
      std::cout.width(50);
      std::cout << "# of OOVs" << perp.NumOOVs() << std::endl;
    }
  }

  return 0;
}
