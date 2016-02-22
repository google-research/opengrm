// ngram-katz.h
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
// Katz backoff derived class for smoothing

#ifndef NGRAM_NGRAM_KATZ_H__
#define NGRAM_NGRAM_KATZ_H__

#include <vector>

#include<ngram/ngram-count-of-counts.h>
#include<ngram/ngram-make.h>

namespace ngram {

class NGramKatz : public NGramMake {
 public:
  // Construct NGramMake object, consisting of the FST and some
  // information about the states under the assumption that the FST is a model.
  // Ownership of the FST is retained by the caller.
  NGramKatz(StdMutableFst *infst,
	    bool backoff = true, Label backoff_label = 0,
	    double norm_eps = kNormEps, bool check_consistency = false,
	    int bins = -1)
      : NGramMake(infst, backoff, backoff_label, norm_eps, check_consistency),
        bins_(bins <= 0 ? 5 : bins),
        count_of_counts_(bins_) {
  }

  // Smooth model according to 'method' and parameters.
  void MakeNGramModel();

  // Pass in count of counts (rather than computing them)
  void SetCountOfCounts(const StdFst &fst) { count_of_counts_.SetCounts(fst); }

 protected:
  // Return negative log discounted count for provided negative log count
  double GetDiscount(double neglogcount, int order) const;

 private:
  // Katz discount for count r: (r*/r - rnorm) / (1 - rnorm)
  // r* = (r+1) n_{r+1} / n_r   and   rnorm = (bins + 1) n_{bins + 1} / n_1
  // stored with indices starting at 0, so bin 0 is count 1
  void CalculateKatzDiscount(int order, int i, double rnorm);

  // Calculate discounts for each order, according to the requested method
  void CalculateDiscounts();

  // Ratio of count mass for lowest undiscounted value to singleton count mass
  // We generalize to allow for count pruning which can lead to no singletons
  double SaneKatzRNorm(int order);

  int bins_;                             // number of bins for discounting
  NGramCountOfCounts count_of_counts_;   // count bins for orders
  std::vector < std::vector <double> > discount_;  // discount for bins

  DISALLOW_COPY_AND_ASSIGN(NGramKatz);
};


}  // namespace ngram

#endif  // NGRAM_NGRAM_KATZ_H__
