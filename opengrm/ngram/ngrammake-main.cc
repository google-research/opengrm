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

// Makes a normalized n-gram model from an input FST with raw counts.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/ngram/hist-arc.h"
#include "opengrm/ngram/ngram-make.h"

ABSL_DECLARE_FLAG(double, witten_bell_k);
ABSL_DECLARE_FLAG(double, discount_D);
ABSL_DECLARE_FLAG(std::string, method);
ABSL_DECLARE_FLAG(bool, backoff);
ABSL_DECLARE_FLAG(bool, interpolate);
ABSL_DECLARE_FLAG(int64_t, bins);
ABSL_DECLARE_FLAG(int64_t, backoff_label);
ABSL_DECLARE_FLAG(double, norm_eps);
ABSL_DECLARE_FLAG(bool, check_consistency);
ABSL_DECLARE_FLAG(std::string, count_of_counts);

int ngrammake_main(int argc, char** argv) {
  std::string usage = "Make n-gram model from input count file.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst [out.fst]]\n";
  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  if (argc < 1 || argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  std::unique_ptr<fst::StdFst> ccfst;
  if (!absl::GetFlag(FLAGS_count_of_counts).empty()) {
    ccfst.reset(fst::StdFst::Read(absl::GetFlag(FLAGS_count_of_counts)));
    if (!ccfst) return 1;
  }

  bool model_made = false;
  std::unique_ptr<fst::StdMutableFst> fst;
  if (absl::GetFlag(FLAGS_method) == "katz_frac") {
    std::unique_ptr<fst::VectorFst<ngram::HistogramArc>> hist_fst(
        fst::VectorFst<ngram::HistogramArc>::Read(in_name));
    if (hist_fst) {
      fst = std::make_unique<fst::StdVectorFst>();
      model_made = ngram::NGramMakeHistModel(
          hist_fst.get(), fst.get(), absl::GetFlag(FLAGS_method), ccfst.get(),
          absl::GetFlag(FLAGS_interpolate), absl::GetFlag(FLAGS_bins),
          absl::GetFlag(FLAGS_backoff_label), absl::GetFlag(FLAGS_norm_eps),
          absl::GetFlag(FLAGS_check_consistency));
    }
  } else {
    fst.reset(fst::StdMutableFst::Read(in_name, /*convert=*/true));
    if (fst) {
      model_made = ngram::NGramMakeModel(
          fst.get(), absl::GetFlag(FLAGS_method), ccfst.get(),
          absl::GetFlag(FLAGS_backoff), absl::GetFlag(FLAGS_interpolate),
          absl::GetFlag(FLAGS_bins), absl::GetFlag(FLAGS_witten_bell_k),
          absl::GetFlag(FLAGS_discount_D), absl::GetFlag(FLAGS_backoff_label),
          absl::GetFlag(FLAGS_norm_eps),
          absl::GetFlag(FLAGS_check_consistency));
    }
  }
  if (model_made) {
    std::string out_name =
        (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "";
    fst->Write(out_name);
  }
  return !model_made;
}
