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

#include "opengrm/ngram/ngram-absolute.h"

#include <cmath>
#include <vector>

#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "absl/log/vlog_is_on.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcsort.h"
#include "opengrm/ngram/ngram-make.h"
#include "opengrm/ngram/ngram-model.h"
#include "opengrm/ngram/util.h"

namespace ngram {

using ::fst::StdArc;
using ::fst::StdILabelCompare;

// Normalize n-gram counts and smooth to create an n-gram model
// Using Absolute Discounting Methods
//  'parameter': discount D
//   number of 'bins' used by Absolute Discounting (>=1)
bool NGramAbsolute::MakeNGramModel() {
  count_of_counts_.CalculateCounts(*this);
  CalculateDiscounts();
  if (VLOG_IS_ON(1)) {
    count_of_counts_.ShowCounts(discount_, "Absolute discounts");
  }
  return NGramMake::MakeNGramModel();
}

// Calculate discounts for each order
void NGramAbsolute::CalculateDiscounts() {
  discount_.clear();
  discount_.resize(HiOrder());

  for (int order = 0; order < HiOrder(); ++order) {
    discount_[order].resize(bins_ + 1, 0.0);  // space for bins + 1
    for (int bin = 0; bin < bins_; ++bin) CalculateAbsoluteDiscount(order, bin);
    // counts higher than largest bin are discounted at largest bin rate
    discount_[order][bins_] = discount_[order][bins_ - 1];
  }
}

// Return negative log discounted count for provided negative log count
double NGramAbsolute::GetDiscount(Weight neglogcount_weight, int order) {
  double neglogcount = ScalarValue(neglogcount_weight);
  double discounted = neglogcount, neglogdiscount;
  if (neglogcount == StdArc::Weight::Zero().Value())  // count = 0
    return neglogcount;
  int bin = count_of_counts_.GetCountBin(neglogcount, bins_, true);
  if (bin >= 0) {
    neglogdiscount = -log(discount_[order][bin]);
    if (neglogdiscount <= neglogcount)              // c - D <= 0
      discounted = StdArc::Weight::Zero().Value();  // set count to 0
    else
      discounted = NegLogDiff(neglogcount, neglogdiscount);  // subtract
  } else {
    NGRAMERROR() << "NGramAbsolute: No discount bin for discounting";
    NGramModel::SetError();
  }
  return discounted;
}

void NGramAbsolute::CalculateAbsoluteDiscount(int order, int bin) {
  if (parameter_ >= 0) {  // user provided discount parameter
    discount_[order][bin] = parameter_;
  } else {  // no discount parameter given: assign based on rule of thumb
    double ROTval = AbsDiscountRuleOfThumb(order);
    if (ROTval <= 0.0) {            // rule of thumb provides unusable parameter
      discount_[order][bin] = 0.6;  // just assign some default parameter
    } else {  // assign according to formula for given rule of thumb value
      discount_[order][bin] = AbsoluteDiscountFormula(order, bin, ROTval);
    }
  }
}

double NGramAbsolute::AbsoluteDiscountFormula(int order, int bin,
                                              double Y) const {
  double discount = bin + 1, n = bin + 2;  // recall bin (k-1) = count k
  n *= Y * count_of_counts_.Count(order, bin + 1);
  if (n == 0.0) n++;  // to avoid full discounts when given an empty bin
  if (count_of_counts_.Count(order, bin) > 0.0)
    n /= count_of_counts_.Count(order, bin);
  discount -= n;
  if (discount <= 0) discount = kNormEps;
  return discount;
}

double NGramAbsolute::AbsDiscountRuleOfThumb(int order) const {
  int basebin = 1;  // cannot assume bins have observations (count pruning)
  while (basebin <= bins_ &&  // find lowest non-zero pair of bins
         (count_of_counts_.Count(order, basebin - 1) <= 0.0 ||
          count_of_counts_.Count(order, basebin) <= 0.0))
    basebin++;
  if (basebin > bins_)  // insufficient non-zero data available in histogram
    return 0.0;
  double k = basebin, kn_k = k * count_of_counts_.Count(order, basebin - 1),
         kp1n_kp1 = (k + 1) * count_of_counts_.Count(order, basebin);
  return kn_k / (kn_k + kp1n_kp1);
}

}  // namespace ngram
