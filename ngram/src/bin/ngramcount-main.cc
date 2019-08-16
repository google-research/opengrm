
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
// Copyright 2005-2016 Brian Roark and Google, Inc.
// Counts n-grams from an input fst archive (FAR) file.

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <fst/log.h>
#include <fst/extensions/far/far.h>
#include <fst/fst.h>
#include <fst/vector-fst.h>
#include <ngram/hist-arc.h>
#include <ngram/ngram-count.h>
#include <ngram/ngram-model.h>

DECLARE_string(method);
DECLARE_int64(order);

// For counting:
DECLARE_bool(round_to_int);
DECLARE_bool(output_fst);
DECLARE_bool(require_symbols);
DECLARE_double(add_to_symbol_unigram_count);

// For counting and histograms:
DECLARE_bool(epsilon_as_backoff);

// For count-of-counting:
DECLARE_string(context_pattern);

// For merging:
DECLARE_double(alpha);
DECLARE_double(beta);
DECLARE_bool(normalize);
DECLARE_int64(backoff_label);
DECLARE_double(norm_eps);
DECLARE_bool(check_consistency);

int ngramcount_main(int argc, char **argv) {
  std::string usage = "Count n-grams from input file.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.far [out.fst]]\n";
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  std::string out_name =
      (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "";

  bool ngrams_counted = false;
  if (FLAGS_method == "counts") {
    std::unique_ptr<fst::FarReader<fst::StdArc>> far_reader(
        fst::FarReader<fst::StdArc>::Open(in_name));
    if (!far_reader) {
      LOG(ERROR) << "ngramcount: open of FST archive failed: " << in_name;
      return 1;
    }
    if (FLAGS_output_fst) {
      fst::StdVectorFst fst;
      ngrams_counted = ngram::GetNGramCounts(
          far_reader.get(), &fst, FLAGS_order, FLAGS_require_symbols,
          FLAGS_epsilon_as_backoff, FLAGS_round_to_int,
          FLAGS_add_to_symbol_unigram_count);
      if (ngrams_counted) fst.Write(out_name);
    } else {
      std::vector<std::string> ngram_counts;
      ngrams_counted = ngram::GetNGramCounts(
          far_reader.get(), &ngram_counts, FLAGS_order,
          FLAGS_epsilon_as_backoff, FLAGS_add_to_symbol_unigram_count);
      std::ofstream ofstrm;
      if (!out_name.empty()) {
        ofstrm.open(out_name);
        if (!ofstrm) {
          LOG(ERROR) << "GetNGramCounts: Open failed, file = " << out_name;
          return 1;
        }
      }
      std::ostream &ostrm = ofstrm.is_open() ? ofstrm : std::cout;
      for (size_t i = 0; i < ngram_counts.size(); ++i)
        ostrm << ngram_counts[i] << std::endl;
    }
  } else if (FLAGS_method == "histograms") {
    std::unique_ptr<fst::FarReader<fst::StdArc>> far_reader(
        fst::FarReader<fst::StdArc>::Open(in_name));
    if (!far_reader) {
      LOG(ERROR) << "ngramhistcount: open of FST archive failed: " << in_name;
      return 1;
    }
    fst::VectorFst<fst::HistogramArc> fst;
    ngrams_counted = ngram::GetNGramHistograms(
        far_reader.get(), &fst, FLAGS_order, FLAGS_epsilon_as_backoff,
        FLAGS_backoff_label, FLAGS_norm_eps, FLAGS_check_consistency,
        FLAGS_normalize, FLAGS_alpha, FLAGS_beta);
    if (ngrams_counted) fst.Write(out_name);
  } else if (FLAGS_method == "count_of_counts" ||
             FLAGS_method == "count_of_histograms") {
    ngrams_counted = true;
    fst::StdVectorFst ccfst;
    if (FLAGS_method == "count_of_counts") {
      std::unique_ptr<fst::StdVectorFst> fst(
          fst::StdVectorFst::Read(in_name));
      if (!fst) return 1;
      ngram::GetNGramCountOfCounts<fst::StdArc>(*fst, &ccfst, FLAGS_order,
                                                    FLAGS_context_pattern);
    } else {
      std::unique_ptr<fst::VectorFst<fst::HistogramArc>> fst(
          fst::VectorFst<fst::HistogramArc>::Read(in_name));
      if (!fst) return 1;
      ngram::GetNGramCountOfCounts<fst::HistogramArc>(
          *fst, &ccfst, FLAGS_order, FLAGS_context_pattern);
    }
    ccfst.Write(out_name);
  } else {
    LOG(ERROR) << argv[0] << ": bad counting method: " << FLAGS_method;
  }
  return !ngrams_counted;
}
