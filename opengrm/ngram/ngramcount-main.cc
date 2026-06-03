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

// Counts n-grams from an input fst archive (FAR) file.

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "openfst/extensions/far/far.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/ngram/hist-arc.h"
#include "opengrm/ngram/ngram-count.h"

ABSL_DECLARE_FLAG(std::string, method);
ABSL_DECLARE_FLAG(int64_t, order);

// For counting:
ABSL_DECLARE_FLAG(bool, round_to_int);
ABSL_DECLARE_FLAG(bool, output_fst);
ABSL_DECLARE_FLAG(bool, require_symbols);
ABSL_DECLARE_FLAG(double, add_to_symbol_unigram_count);

// For counting and histograms:
ABSL_DECLARE_FLAG(bool, epsilon_as_backoff);

// For count-of-counting:
ABSL_DECLARE_FLAG(std::string, context_pattern);

// For merging:
ABSL_DECLARE_FLAG(double, alpha);
ABSL_DECLARE_FLAG(double, beta);
ABSL_DECLARE_FLAG(bool, normalize);
ABSL_DECLARE_FLAG(int64_t, backoff_label);
ABSL_DECLARE_FLAG(double, norm_eps);
ABSL_DECLARE_FLAG(bool, check_consistency);

int ngramcount_main(int argc, char** argv) {
  std::string usage = "Count n-grams from input file.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.far [out.fst]]\n";
  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  std::string out_name =
      (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "";

  bool ngrams_counted = false;
  if (absl::GetFlag(FLAGS_method) == "counts") {
    std::unique_ptr<fst::FarReader<fst::StdArc>> far_reader(
        fst::FarReader<fst::StdArc>::Open(in_name));
    if (!far_reader) {
      LOG(ERROR) << "ngramcount: open of FST archive failed: " << in_name;
      return 1;
    }
    if (absl::GetFlag(FLAGS_output_fst)) {
      fst::StdVectorFst fst;
      ngrams_counted = ngram::GetNGramCounts(
          far_reader.get(), &fst, absl::GetFlag(FLAGS_order),
          absl::GetFlag(FLAGS_require_symbols),
          absl::GetFlag(FLAGS_epsilon_as_backoff),
          absl::GetFlag(FLAGS_round_to_int),
          absl::GetFlag(FLAGS_add_to_symbol_unigram_count));
      if (ngrams_counted) fst.Write(out_name);
    } else {
      std::vector<std::string> ngram_counts;
      ngrams_counted = ngram::GetNGramCounts(
          far_reader.get(), &ngram_counts, absl::GetFlag(FLAGS_order),
          absl::GetFlag(FLAGS_epsilon_as_backoff),
          absl::GetFlag(FLAGS_add_to_symbol_unigram_count));
      std::ofstream ofstrm;
      if (!out_name.empty()) {
        ofstrm.open(out_name);
        if (!ofstrm) {
          LOG(ERROR) << "GetNGramCounts: Open failed, file = " << out_name;
          return 1;
        }
      }
      std::ostream& ostrm = ofstrm.is_open() ? ofstrm : std::cout;
      for (size_t i = 0; i < ngram_counts.size(); ++i)
        ostrm << ngram_counts[i] << std::endl;
    }
  } else if (absl::GetFlag(FLAGS_method) == "histograms") {
    std::unique_ptr<fst::FarReader<fst::StdArc>> far_reader(
        fst::FarReader<fst::StdArc>::Open(in_name));
    if (!far_reader) {
      LOG(ERROR) << "ngramhistcount: open of FST archive failed: " << in_name;
      return 1;
    }
    fst::VectorFst<ngram::HistogramArc> fst;
    ngrams_counted = ngram::GetNGramHistograms(
        far_reader.get(), &fst, absl::GetFlag(FLAGS_order),
        absl::GetFlag(FLAGS_epsilon_as_backoff),
        absl::GetFlag(FLAGS_backoff_label), absl::GetFlag(FLAGS_norm_eps),
        absl::GetFlag(FLAGS_check_consistency), absl::GetFlag(FLAGS_normalize),
        absl::GetFlag(FLAGS_alpha), absl::GetFlag(FLAGS_beta));
    if (ngrams_counted) fst.Write(out_name);
  } else if (absl::GetFlag(FLAGS_method) == "count_of_counts" ||
             absl::GetFlag(FLAGS_method) == "count_of_histograms") {
    ngrams_counted = true;
    fst::StdVectorFst ccfst;
    if (absl::GetFlag(FLAGS_method) == "count_of_counts") {
      std::unique_ptr<fst::StdFst> fst(fst::StdFst::Read(in_name));
      if (!fst) return 1;
      ngram::GetNGramCountOfCounts<fst::StdArc>(
          *fst, &ccfst, absl::GetFlag(FLAGS_order),
          absl::GetFlag(FLAGS_context_pattern));
    } else {
      std::unique_ptr<fst::VectorFst<ngram::HistogramArc>> fst(
          fst::VectorFst<ngram::HistogramArc>::Read(in_name));
      if (!fst) return 1;
      ngram::GetNGramCountOfCounts<ngram::HistogramArc>(
          *fst, &ccfst, absl::GetFlag(FLAGS_order),
          absl::GetFlag(FLAGS_context_pattern));
    }
    ccfst.Write(out_name);
  } else {
    LOG(ERROR) << argv[0]
               << ": bad counting method: " << absl::GetFlag(FLAGS_method);
  }
  return !ngrams_counted;
}
