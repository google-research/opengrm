// Copyright 2005-2013 Brian Roark
// Copyright 2005-2020 Google LLC
//
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
// Makes a normalized n-gram model from an input FST with raw counts.

#include <memory>
#include <string>

#include <fst/flags.h>
#include <ngram/hist-arc.h>
#include <ngram/ngram-make.h>

DECLARE_double(witten_bell_k);
DECLARE_double(discount_D);
DECLARE_string(method);
DECLARE_bool(backoff);
DECLARE_bool(interpolate);
DECLARE_int64(bins);
DECLARE_int64(backoff_label);
DECLARE_double(norm_eps);
DECLARE_bool(check_consistency);
DECLARE_string(count_of_counts);

int ngrammake_main(int argc, char **argv) {
  std::string usage = "Make n-gram model from input count file.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst [out.fst]]\n";
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc < 1 || argc > 3) {
    ShowUsage();
    return 1;
  }

  std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  std::unique_ptr<fst::StdFst> ccfst;
  if (!FLAGS_count_of_counts.empty()) {
    ccfst.reset(fst::StdFst::Read(FLAGS_count_of_counts));
    if (!ccfst) return 1;
  }

  bool model_made = false;
  std::unique_ptr<fst::StdVectorFst> fst;
  if (FLAGS_method == "katz_frac") {
    std::unique_ptr<fst::VectorFst<ngram::HistogramArc>> hist_fst(
        fst::VectorFst<ngram::HistogramArc>::Read(in_name));
    if (hist_fst) {
      fst.reset(new fst::StdVectorFst());
      model_made = ngram::NGramMakeHistModel(
          hist_fst.get(), fst.get(), FLAGS_method, ccfst.get(),
          FLAGS_interpolate, FLAGS_bins,
          FLAGS_backoff_label, FLAGS_norm_eps,
          FLAGS_check_consistency);
    }
  } else {
    fst.reset(fst::StdVectorFst::Read(in_name));
    if (fst) {
      model_made = ngram::NGramMakeModel(
          fst.get(), FLAGS_method, ccfst.get(), FLAGS_backoff,
          FLAGS_interpolate, FLAGS_bins,
          FLAGS_witten_bell_k, FLAGS_discount_D,
          FLAGS_backoff_label, FLAGS_norm_eps,
          FLAGS_check_consistency);
    }
  }
  if (model_made) {
    std::string out_name =
        (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "";
    fst->Write(out_name);
  }
  return !model_made;
}
