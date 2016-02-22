// ngram-count-of-counts.h
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
// Class for computing/accessing count-of-count bins for
// e.g., Katz and absolute discounting.

#ifndef NGRAM_NGRAM_COUNT_OF_COUNTS_H__
#define NGRAM_NGRAM_COUNT_OF_COUNTS_H__

#include <vector>

#include <fst/mutable-fst.h>
#include <ngram/ngram-context.h>
#include <ngram/ngram-model.h>

namespace ngram {

using fst::StdMutableFst;
using fst::StdFst;
using std::ostringstream;
using fst::SymbolTable;

class NGramCountOfCounts {
 public:
  typedef StdArc::StateId StateId;
  typedef StdArc::Label Label;
  typedef StdArc::Weight Weight;

  static const int kMaxBins;  // maximum # of bins allowed

  explicit NGramCountOfCounts(int bins = -1)
      : bins_(bins < 0 ? kMaxBins : bins) {
    if (bins_ > kMaxBins)
      LOG(FATAL) << "NGramCountOfCounts: Number of bins too large: " << bins_;
  }

  NGramCountOfCounts(string context_pattern, int order, int bins = -1)
      : bins_(bins < 0 ? kMaxBins : bins),
        context_(context_pattern, order) {
    if (bins_ > kMaxBins)
      LOG(FATAL) << "NGramCountOfCounts: Number of bins too large: " << bins_;
  }

  NGramCountOfCounts(const std::vector<Label> &context_begin,
                     const std::vector<Label> &context_end,
                     int order, int bins = -1)
      : bins_(bins < 0 ? kMaxBins : bins),
        context_(context_begin, context_end, order) {
    if (bins_ > kMaxBins)
      LOG(FATAL) << "NGramCountOfCounts: Number of bins too large: " << bins_;
  }

  // Calculate count histograms
  void CalculateCounts(const NGramModel &model);

  // Returns the number of bins
  int GetBins() const { return bins_;  }

  // Put ngram in bin = count - 1 for 0 < count <= bins
  // include big counts when discounting, but not when building histograms
  int GetCountBin(double weight, int bins, bool includebig) const {
    int wt = round(exp(-weight)) - 1;  // rounding count to integer
    if (wt < 0 || (!includebig && wt > bins))  // if bin should not be assigned
      wt = -1;
    else if (wt > bins)  // include big counts in highest bin discounting
      wt = bins;
    return wt;
  }

  // NB: unigram is order 0 here, etc.
  double Count(int order, int bin) const { return histogram_[order][bin]; }

  // Display input histogram
  void ShowCounts(const std::vector < std::vector <double> > &hist,
                  const string &label) const;

  // Display internal histogram
  void ShowCounts(const string &label) const {
    ShowCounts(histogram_, label);
  }

  // Get an Fst representation of the ngram count-of-counts
  void GetFst(StdMutableFst *fst) const;

  // Sets counts from count-of-counts FST
  void SetCounts(const StdFst &fst);

 private:
  // Find bin for the value provided and increment the histogram for that bin
  void IncrementBinCount(int order, double value) {
    int bin = GetCountBin(value, GetBins(), false);
    if (bin >= 0)
      ++histogram_[order][bin];
  }

  std::vector < std::vector <double> > histogram_;  // count histogram for orders
  int bins_;                              // Number of bins for discounting
  NGramContext context_;                  // context specification
  DISALLOW_COPY_AND_ASSIGN(NGramCountOfCounts);
};

}  // namespace ngram

#endif  // NGRAM_NGRAM_COUNT_OF_COUNTS_H__
