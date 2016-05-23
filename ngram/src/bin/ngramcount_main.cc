
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
// Copyright 2005-2016 Brian Roark and Google, Inc.
// Counts n-grams from an input fst archive (FAR) file.

#include <fstream>
#include <ostream>
#include <string>
#include <vector>

#include <fst/extensions/far/far.h>
#include <fst/arcsort.h>
#include <fst/fst.h>
#include <fst/map.h>
#include <fst/vector-fst.h>
#include <ngram/hist-arc.h>
#include <ngram/ngram-count-of-counts.h>
#include <ngram/ngram-count.h>
#include <ngram/ngram-hist-merge.h>
#include <ngram/ngram-model.h>

DEFINE_string(method, "counts",
              "One of: \"counts\", \"histograms\", \"count_of_counts\", "
              "\"count_of_histograms\"");
DEFINE_int64(order, 3, "Set maximal order of ngrams to be counted");

// For counting:
DEFINE_bool(round_to_int, false, "Round all counts to integers");
DEFINE_bool(output_fst, true, "Output counts as fst (otherwise strings)");
DEFINE_bool(require_symbols, true, "Require symbol tables? (default: yes)");

// For counting and histograms:
DEFINE_bool(epsilon_as_backoff, false,
            "Treat epsilon in the input Fsts as backoff");

// For count-of-counting:
DEFINE_string(context_pattern, "", "Pattern of contexts to count");

// For merging:
DEFINE_double(alpha, 1.0, "Weight for first FST");
DEFINE_double(beta, 1.0, "Weight for second (and subsequent) FST(s)");
DEFINE_bool(normalize, false, "Normalize resulting model");
DEFINE_int64(backoff_label, 0, "Backoff label");
DEFINE_double(norm_eps, ngram::kNormEps, "Normalization check epsilon");
DEFINE_bool(check_consistency, false, "Check model consistency");

namespace fst {

typedef LogWeightTpl<double> Log64Weight;
typedef ArcTpl<Log64Weight> Log64Arc;

// Log64Arc Mapper
template <class Arc>
struct ToLog64Mapper {
  typedef Arc FromArc;
  typedef Log64Arc ToArc;

  ToArc operator()(const FromArc &arc) const {
    return ToArc(arc.ilabel, arc.olabel, arc.weight.Value(), arc.nextstate);
  }

  MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }
  MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS; }
  MapSymbolsAction OutputSymbolsAction() const { return MAP_COPY_SYMBOLS; }
  uint64 Properties(uint64 props) const { return props; }
};

// Histogram Mapper
template <class Arc>
class ToHistogramMapper {
 public:
  typedef Arc FromArc;
  typedef HistogramArc ToArc;

  ToArc operator()(const Arc &arc) const {
    std::vector<TropicalWeight> v(ngram::kHistogramBins,
                                  TropicalWeight::Zero());
    double val = arc.weight.Value();
    v[0] = val;
    double round_down = floor(exp(-val));
    double round_up = round_down + 1;
    int index = static_cast<int>(round_up);
    if (index < ngram::kHistogramBins - 1) {
      v[index + 1] = -log(exp(-val) - round_down);
    }
    if (index && index < ngram::kHistogramBins) {
      v[index] = -log(round_up - exp(-val));
    }
    return ToArc(
        arc.ilabel, arc.olabel,
        PowerWeight<TropicalWeight, ngram::kHistogramBins>(v.begin(), v.end()),
        arc.nextstate);
  }
  MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }
  MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS; }
  MapSymbolsAction OutputSymbolsAction() const { return MAP_COPY_SYMBOLS; }
  uint64 Properties(uint64 props) const { return props; }
};

}  // namespace fst

// Get ngram counts for the next fst in far_reader
bool GetCounts(ngram::NGramCounter<fst::Log64Weight> *ngram_counter,
               fst::StdMutableFst *fst,
               fst::FarReader<fst::StdArc> *far_reader, int fstnumber) {
  std::unique_ptr<const fst::StdVectorFst> ifst(
      new fst::StdVectorFst(*far_reader->GetFst()));

  if (!ifst) {
    LOG(ERROR) << "ngramhistcount: unable to read fst #" << fstnumber;
    return false;
  }

  bool counted = false;
  if (ifst->Properties(fst::kString | fst::kUnweighted, true)) {
    counted = ngram_counter->Count(*ifst);
  } else {
    fst::VectorFst<fst::Log64Arc> log_ifst;
    Map(*ifst, &log_ifst, fst::ToLog64Mapper<fst::StdArc>());
    counted = ngram_counter->Count(&log_ifst);
  }
  if (!counted) {
    LOG(ERROR) << "ngramhistcount: fst #" << fstnumber << " skipped";
  }

  ngram_counter->GetFst(fst);
  fst::ArcSort(fst, fst::StdILabelCompare());
  if (ifst->InputSymbols() != 0) {
    fst->SetInputSymbols(ifst->InputSymbols());
    fst->SetOutputSymbols(ifst->InputSymbols());
  }

  return true;
}

// Compute counts
int GetNGramHistograms(const string &in_name, const string &out_name) {
  std::unique_ptr<fst::FarReader<fst::StdArc>> far_reader(
      fst::FarReader<fst::StdArc>::Open(in_name));
  if (!far_reader) {
    LOG(ERROR) << "ngramhistcount: open of FST archive failed: " << in_name;
    return 1;
  }

  int fstnumber = 1;

  ngram::NGramCounter<fst::Log64Weight> ngram_counter(
      FLAGS_order, FLAGS_epsilon_as_backoff);
  if (ngram_counter.Error()) {
    return 1;
  }
  fst::StdVectorFst fst1;
  if (!far_reader->Done() &&
      !GetCounts(&ngram_counter, &fst1, far_reader.get(), fstnumber)) {
    return 1;
  }

  fst::VectorFst<ngram::HistogramArc> hist_fst1;
  Map(fst1, &hist_fst1, fst::ToHistogramMapper<fst::StdArc>());
  ngram::NGramHistMerge ngramrg(&hist_fst1, FLAGS_backoff_label, FLAGS_norm_eps,
                                FLAGS_check_consistency);

  far_reader->Next();
  ++fstnumber;

  while (!far_reader->Done()) {
    ngram::NGramCounter<fst::Log64Weight> ngram_counter(
        FLAGS_order, FLAGS_epsilon_as_backoff);
    fst::StdVectorFst fst;
    if (!GetCounts(&ngram_counter, &fst, far_reader.get(), fstnumber)) {
      return 1;
    }

    fst::VectorFst<ngram::HistogramArc> hist_fst;
    Map(fst, &hist_fst, fst::ToHistogramMapper<fst::StdArc>());
    bool norm = FLAGS_normalize && far_reader->Done();
    ngramrg.MergeNGramModels(hist_fst, FLAGS_alpha, FLAGS_beta, norm);

    far_reader->Next();
    ++fstnumber;
  }

  ngramrg.GetFst().Write(out_name);

  return 0;
}

// Rounds -log count to values corresponding to the rounded integer count
// Reduces small floating point precision issues when dealing with int counts
// Primarily for testing that methods for deriving the same model are identical
void RoundCountsToInt(fst::StdMutableFst *fst) {
  for (size_t s = 0; s < fst->NumStates(); ++s) {
    for (fst::MutableArcIterator<fst::StdMutableFst> aiter(fst, s);
         !aiter.Done(); aiter.Next()) {
      fst::StdArc arc = aiter.Value();
      int weight = round(exp(-arc.weight.Value()));
      arc.weight = -log(weight);
      aiter.SetValue(arc);
    }
    if (fst->Final(s) != fst::StdArc::Weight::Zero()) {
      int weight = round(exp(-fst->Final(s).Value()));
      fst->SetFinal(s, -log(weight));
    }
  }
}

double GetNGramAndCount(
    const std::pair<std::vector<int>, std::pair<int, double>> &ngram_count,
    string *ngram, const fst::SymbolTable &syms) {
  std::vector<int> ngram_history = ngram_count.first;
  *ngram = "";
  for (size_t i = 0; i < ngram_history.size(); ++i) {
    string symbol = ngram_history[i] > 0 ? syms.Find(ngram_history[i]) : "<s>";
    *ngram += symbol + " ";
  }
  if (ngram_count.second.first > 0) {
    *ngram += syms.Find(ngram_count.second.first);
  } else {
    *ngram += "</s>";
  }
  return ngram_count.second.second;
}

// Compute counts
int GetNGramCounts(const string &in_name, const string &out_name,
                   bool get_fst) {
  ngram::NGramCounter<fst::Log64Weight> ngram_counter(
      FLAGS_order, FLAGS_epsilon_as_backoff);

  fst::FstReadOptions opts;
  std::unique_ptr<fst::FarReader<fst::StdArc>> far_reader(
      fst::FarReader<fst::StdArc>::Open(in_name));
  if (!far_reader) {
    LOG(ERROR) << "ngramcount: open of FST archive failed: " << in_name;
    return 1;
  }

  int fstnumber = 1;
  std::unique_ptr<const fst::StdVectorFst> ifst;
  std::unique_ptr<const fst::StdVectorFst> lfst;
  while (!far_reader->Done()) {
    ifst.reset(new fst::StdVectorFst(*far_reader->GetFst()));
    if (!ifst) {
      LOG(ERROR) << "ngramcount: unable to read fst #" << fstnumber;
      return 1;
    }

    bool counted = false;
    if (ifst->Properties(fst::kString | fst::kUnweighted, true)) {
      counted = ngram_counter.Count(*ifst);
    } else {
      fst::VectorFst<fst::Log64Arc> log_ifst;
      Map(*ifst, &log_ifst, fst::ToLog64Mapper<fst::StdArc>());
      counted = ngram_counter.Count(&log_ifst);
    }
    if (!counted) LOG(ERROR) << "ngramcount: fst #" << fstnumber << " skipped";

    if (ifst->InputSymbols() != 0) {  // retain for symbol table.
      lfst = std::move(ifst);
    }
    far_reader->Next();
    ++fstnumber;
  }

  if ((FLAGS_require_symbols || !get_fst) && !lfst) {
    LOG(ERROR) << "None of the input FSTs had a symbol table";
    return 1;
  }

  if (get_fst) {
    fst::StdVectorFst fst;
    ngram_counter.GetFst(&fst);
    fst::ArcSort(&fst, fst::StdILabelCompare());
    if (lfst) {
      fst.SetInputSymbols(lfst->InputSymbols());
      fst.SetOutputSymbols(lfst->InputSymbols());
    }
    if (FLAGS_round_to_int) RoundCountsToInt(&fst);
    fst.Write(out_name);
  } else {
    std::vector<std::pair<std::vector<int>, std::pair<int, double>>>
        ngram_counts;
    ngram_counter.GetReverseContextNGrams<fst::StdArc>(&ngram_counts);
    std::ofstream ofstrm;
    if (!out_name.empty()) {
      ofstrm.open(out_name);
      if (!ofstrm) {
        LOG(ERROR) << "GetNGramCounts: Open failed, file = " << out_name;
        return 1;
      }
    }
    std::ostream &ostrm = ofstrm.is_open() ? ofstrm : std::cout;
    for (size_t i = 0; i < ngram_counts.size(); ++i) {
      string ngram;
      double count =
          GetNGramAndCount(ngram_counts[i], &ngram, *lfst->InputSymbols());
      ostrm << ngram << '\t' << count << std::endl;
    }
  }
  return 0;
}

// Compute count-of-counts
template <class Arc>
int GetNGramCountOfCounts(const string &in_name, const string &out_name) {
  std::unique_ptr<fst::VectorFst<Arc>> fst(
      fst::VectorFst<Arc>::Read(in_name));
  if (!fst) return 1;
  ngram::NGramModel<Arc> ngram(*fst, 0, ngram::kNormEps,
                               !FLAGS_context_pattern.empty());
  int order = ngram.HiOrder() > FLAGS_order ? ngram.HiOrder() : FLAGS_order;
  ngram::NGramCountOfCounts<Arc> count_of_counts(FLAGS_context_pattern, order);
  count_of_counts.CalculateCounts(ngram);
  fst::StdVectorFst count_of_counts_fst;
  count_of_counts.GetFst(&count_of_counts_fst);
  count_of_counts_fst.Write(out_name);

  return 0;
}

int main(int argc, char **argv) {
  string usage = "Count ngram from input file.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.far [out.fst]]\n";
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  string in_name = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  string out_name = (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "";

  if (FLAGS_method == "counts") {
    return GetNGramCounts(in_name, out_name, FLAGS_output_fst);
  } else if (FLAGS_method == "count_of_counts") {
    return GetNGramCountOfCounts<fst::StdArc>(in_name, out_name);
  } else if (FLAGS_method == "histograms") {
    return GetNGramHistograms(in_name, out_name);
  } else if (FLAGS_method == "count_of_histograms") {
    return GetNGramCountOfCounts<ngram::HistogramArc>(in_name, out_name);
  } else {
    LOG(ERROR) << argv[0] << ": bad counting method: " << FLAGS_method;
    return 1;
  }
}
