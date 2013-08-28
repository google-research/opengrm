// ngram-katz.cc
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

#include <vector>

#include <fst/arcsort.h>
#include <fst/vector-fst.h>

#include <ngram/ngram-katz.h>

namespace ngram {

using std::vector;

using fst::VectorFst;
using fst::StdILabelCompare;

// Normalize n-gram counts and smooth to create an n-gram model
// Using Katz smoothing methods
//   number of 'bins' used by Katz (>=1)
void NGramKatz::MakeNGramModel() {
  count_of_counts_.CalculateCounts(*this);
  CalculateDiscounts();
  if (FLAGS_v > 0)
    count_of_counts_.ShowCounts(discount_, "Katz discounts");
  NGramMake::MakeNGramModel();
}

// Katz discount for count r: (r*/r - rnorm) / (1 - rnorm)
// r* = (r+1) n_{r+1} / n_r   and   rnorm = (bins + 1) n_{bins + 1} / n_1
// stored with indices starting at 0, so bin 0 is count 1
void NGramKatz::CalculateKatzDiscount(int order, int bin, double rnorm) {
  if (bin < bins_) {
    double rstar = bin + 2, denom = 1.0 - rnorm;
    if (count_of_counts_.Count(order, bin + 1) > 0.0)
      rstar *= count_of_counts_.Count(order, bin + 1);
    if (count_of_counts_.Count(order, bin) > 0.0)
      rstar /= count_of_counts_.Count(order, bin);
    discount_[order][bin] = rstar;
    discount_[order][bin] /= bin + 1;
    discount_[order][bin] -= rnorm;
    if (denom > 0.0)
      discount_[order][bin] /= denom;
  }
  if (bin == bins_ || discount_[order][bin] < 0.0 || 
      discount_[order][bin] >= 1.0) {  // need to provide epsilon discount
    if (bin < bins_ && discount_[order][bin] != 1.0)
      VLOG(1) << "Histograms violating Good-Turing assumptions";
    discount_[order][bin] = 1.0;
    discount_[order][bin] -= kNormEps;  // everything discounted epsilon
  }
}

// Calculate Katz discounts for each order
void NGramKatz::CalculateDiscounts() {
  discount_.clear();
  discount_.resize(HiOrder());

  for (int order = 0; order < HiOrder(); ++order) {
    discount_[order].resize(bins_ + 1, 0.0);       // space for bins + 1
    double rnorm = SaneKatzRNorm(order);
    for (int bin = 0; bin <= bins_; ++bin)
      CalculateKatzDiscount(order, bin, rnorm);
  }
}

// Ratio of count mass for lowest undiscounted value to singleton count mass
// We generalize to allow for count pruning which can lead to zero singletons
// standard form: rnorm = ( bins_ + 1 ) n_{bins_ + 1} / n_1
// generalized: rnorm = ( bins_ + 1 ) n_{bins_ + 1} / ( k n_k )
//              for k = lowest count with non-zero observations
double NGramKatz::SaneKatzRNorm(int order) {
  double rnorm_numerator = bins_ + 1, rnorm_denom = 1;
  rnorm_numerator *= count_of_counts_.Count(order, bins_);
  int rlevel = 0;
  while (rlevel < bins_ && count_of_counts_.Count(order, rlevel) <= 0) {
    ++rlevel;
    ++rnorm_denom;
  }
  rnorm_denom *= count_of_counts_.Count(order, rlevel);
  return rnorm_numerator / rnorm_denom;
}

// Return negative log discounted count for provided negative log count
double NGramKatz::GetDiscount(double neglogcount, int order) const {
  double discounted = neglogcount, neglogdiscount;
  if (neglogcount == StdArc::Weight::Zero().Value())  // count = 0
    return neglogcount;
  int bin = count_of_counts_.GetCountBin(neglogcount, bins_, true);
  if (bin >= 0) {
    neglogdiscount = -log(discount_[order][bin]);
    discounted += neglogdiscount;  // multiply discount times raw count
  } else {
    LOG(FATAL) << "NGramKatz: No discount bin for discounting";
  }
  return discounted;
}

}  // namespace ngram
