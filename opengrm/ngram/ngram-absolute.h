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

// Absolute Discounting derived class for smoothing.

#ifndef OPENGRM_NGRAM_NGRAM_ABSOLUTE_H_
#define OPENGRM_NGRAM_NGRAM_ABSOLUTE_H_

#include <vector>

#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"
#include "opengrm/ngram/ngram-count-of-counts.h"
#include "opengrm/ngram/ngram-make.h"
#include "opengrm/ngram/ngram-model.h"

namespace ngram {

class NGramAbsolute : public NGramMake<fst::StdArc> {
 public:
  // Construct NGramMake object, consisting of the FST and some
  // information about the states under the assumption that the FST is a model.
  // Ownership of the FST is retained by the caller.
  explicit NGramAbsolute(fst::StdMutableFst* infst, bool backoff = false,
                         Label backoff_label = 0, double norm_eps = kNormEps,
                         bool check_consistency = false,
                         double parameter = -1.0, int bins = -1)
      : NGramMake(infst, backoff, backoff_label, norm_eps, check_consistency),
        parameter_(parameter),
        bins_(bins <= 0 ? 1 : bins),
        count_of_counts_(bins_) {}

  // Smooth model according to 'method' and parameters.
  bool MakeNGramModel() override;

  // Pass in count of counts (rather than computing them)
  void SetCountOfCounts(const fst::StdFst& fst) {
    count_of_counts_.SetCounts(fst);
  }

 protected:
  // Return negative log discounted count for provided negative log count
  double GetDiscount(Weight neglogcount, int order) override;

 private:
  // Calculate absolute discount parameter for count i
  // Note: discounts stored with bin indices starting at 0, bin k is count k+1
  void CalculateAbsoluteDiscount(int order, int bin);

  // Calculate absolute discounting parameter according to histogram formula.
  // Using Chen and Goodman version from equation (26) of paper
  // For count i, discount: i - ( (i+1) Y n_{i+1} / n_{i} ) for a given Y
  double AbsoluteDiscountFormula(int order, int bin, double Y) const;

  // Generalized rule of thumb: Y = k n_k / ( k n_k + (k+1) * n_{k+1} )
  // where n_k is the total count mass for items that occurred k times
  // Note: method generalized to allow for zeros in low count bins:
  //       find lowest non-empty count bins, then use rule of thumb
  double AbsDiscountRuleOfThumb(int order) const;

  // Calculate discounts for each order, according to the requested method
  void CalculateDiscounts();

  double parameter_;  // Absolute Discounting D
  int bins_;          // number of bins for discounting
  NGramCountOfCounts<fst::StdArc> count_of_counts_;  // count bins for orders
  std::vector<std::vector<double> > discount_;       // discount for bins
};

}  // namespace ngram

#endif  // OPENGRM_NGRAM_NGRAM_ABSOLUTE_H_
