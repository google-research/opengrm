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

// Computes the perplexity for a stochastic FST. The FST must be normalized.
// The FST must be topologically sorted and have a single path.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ios>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "openfst/extensions/far/far.h"
#include "openfst/lib/fst-decl.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/util.h"
#include "opengrm/sfst/perplexity.h"

ABSL_DECLARE_FLAG(int64_t, phi_label);
ABSL_DECLARE_FLAG(int64_t, unknown_label);
ABSL_DECLARE_FLAG(bool, detailed);
ABSL_DECLARE_FLAG(double, delta);
ABSL_DECLARE_FLAG(double, entropy_delta);

namespace {
constexpr double kSkippedDelta = 1.0e-6;
}

int sfstperplexity_main(int argc, char** argv) {
  using fst::FarReader;
  using fst::PrintField;
  using fst::StdArc;
  using fst::StdFst;
  using sfst::Perplexity;

  std::string usage = "Computes perplexity for SFSTs.\n\n  Usage: ";
  usage += argv[0];
  usage +=
      " q.fst p.{fst,far}   (cross perplexity w.r.t. exp(-p log q))\n         ";
  usage += argv[0];
  usage += " p.{fst,far}         (self perplexity)\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc < 2 || argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  std::string far_name = argc == 3 ? argv[2] : argv[1];

  Perplexity<StdArc> perp(
      absl::GetFlag(FLAGS_phi_label), absl::GetFlag(FLAGS_unknown_label),
      absl::GetFlag(FLAGS_delta), absl::GetFlag(FLAGS_entropy_delta));
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
    if (absl::GetFlag(FLAGS_detailed)) {
      PrintField(std::cout, "source", nsent);
      if (perp.SkipCount() > kSkippedDelta) {
        PrintField(std::cout, "skip count", perp.SkipCount());
      }
      PrintField(std::cout, entropy_type, perp.GetEntropy());
      PrintField(std::cout, "perplexity/symbol", perp.GetPerplexity());
      if (perp.NumOOVs() > 0) {
        PrintField(std::cout, "# of OOVs", perp.NumOOVs());
      }
      perp.Reset();
    }
  }

  if (!absl::GetFlag(FLAGS_detailed)) {
    PrintField(std::cout, "# of sources", perp.NumSources());
    if (perp.SkipCount() > kSkippedDelta * perp.NumSources()) {
      PrintField(std::cout, "skip count", perp.SkipCount());
    }
    if (perp.SkipCount() < perp.NumSources() + kSkippedDelta) {
      PrintField(std::cout, entropy_type, perp.GetEntropy());
      PrintField(std::cout, "perplexity/symbol", perp.GetPerplexity());
      if (perp.NumOOVs() > 0) {
        PrintField(std::cout, "# of OOVs", perp.NumOOVs());
      }
    }
  }

  return 0;
}
