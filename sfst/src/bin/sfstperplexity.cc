
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
             "Unknown symbol label (determines OOV handling)");
DEFINE_bool(detailed, false, "Compute perplexity per source");
DEFINE_double(delta, fst::kDelta, "Comparison delta");
DEFINE_double(entropy_delta, sfst::kEntropyDelta,
              "Convergence delta for entropy/perplexity algorithms");

constexpr double kSkippedDelta = 1.0e-6;

int main(int argc, char **argv) {
  using fst::FarReader;
  using fst::StdArc;
  using fst::StdFst;
  using sfst::Perplexity;

  std::string usage = "Computes perplexity for SFSTs.\n\n  Usage: ";
  usage += argv[0];
  usage += " q.fst p.{fst,far}   (cross perplexity w.r.t. -p log q)\n         ";
  usage += argv[0];
  usage += " p.{fst,far}         (self perplexity)\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc < 2 || argc > 3) {
    ShowUsage();
    return 1;
  }

  std::string far_name = argc == 3 ? argv[2] : argv[1];

  Perplexity<StdArc> perp(FLAGS_phi_label, FLAGS_unknown_label,
                          FLAGS_delta, FLAGS_entropy_delta);
  std::string entropy_type = "self entropy/source";

  if (argc == 3) {
    std::unique_ptr<StdFst> fst(StdFst::Read(argv[1]));
    if (!fst) return 1;
    perp.SetTarget(*fst);
    entropy_type = "cross entropy/source";
  }

  std::unique_ptr<FarReader<StdArc>> far_reader(
      FarReader<StdArc>::Open(far_name));
  if (!far_reader) {
    LOG(ERROR) << argv[0] << ": Can't open source FST/FAR file: " << far_name;
    return 1;
  }

  std::cout.setf(std::ios::left);
  size_t nsent = 0;
  for (++nsent; !far_reader->Done(); ++nsent, far_reader->Next()) {
    if (!perp.Apply(*far_reader->GetFst())) return 1;
    if (FLAGS_detailed) {
      std::cout.width(50);
      std::cout << "source" << nsent << std::endl;
      std::cout.width(50);
      if (perp.SkipCount() > kSkippedDelta)
        std::cout << "skip count" << perp.SkipCount() << std::endl;
      std::cout.width(50);
      std::cout << entropy_type << perp.GetEntropy() << std::endl;
      std::cout.width(50);
      std::cout << "perplexity/symbol" << perp.GetPerplexity() << std::endl;
      if (perp.NumOOVs() > 0) {
        std::cout.width(50);
        std::cout << "# of OOVs" << perp.NumOOVs() << std::endl;
      }
      perp.Reset();
    }
  }

  if (!FLAGS_detailed) {
    std::cout.width(50);
    std::cout << "# of sources" << perp.NumSources() << std::endl;
    if (perp.SkipCount() > kSkippedDelta * perp.NumSources()) {
      std::cout.width(50);
      std::cout << "skip count" << perp.SkipCount() << std::endl;
    }
    if (perp.SkipCount() < perp.NumSources() + kSkippedDelta) {
      std::cout.width(50);
      std::cout << entropy_type << perp.GetEntropy() << std::endl;
      std::cout.width(50);
      std::cout << "perplexity/symbol" << perp.GetPerplexity() << std::endl;
      if (perp.NumOOVs() > 0) {
        std::cout.width(50);
        std::cout << "# of OOVs" << perp.NumOOVs() << std::endl;
      }
    }
  }

  return 0;
}
