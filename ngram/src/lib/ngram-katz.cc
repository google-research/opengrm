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
// Copyright 2009-2011 Brian Roark and Google, Inc.
// Authors: roarkbr@gmail.com  (Brian Roark)
//          allauzen@google.com (Cyril Allauzen)
//          riley@google.com (Michael Riley)
//
// \file

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
  if (bins_ <= 0) bins_ = 5;  // default discounting bins for Katz = 5
  InitializeCountBins(&histogram_, bins_);
  InitializeCountBins(&discount_, bins_);
  CalculateHistograms();
  CalculateDiscounts();
  if (FLAGS_v > 0)
    ShowDiscounts(&discount_, "Katz", bins_);
  NGramMake::MakeNGramModel();
}
    
// Katz discount for count r: (r*/r - rnorm) / (1 - rnorm)
// r* = (r+1) n_{r+1} / n_r   and   rnorm = (bins + 1) n_{bins + 1} / n_1
// stored with indices starting at 0, so bin 0 is count 1
void NGramKatz::CalculateKatzDiscount(int order, int bin, double rnorm) {
  double rstar = bin + 2, norm = 1;
  rstar *= histogram_[order][bin + 1];
  rstar /= histogram_[order][bin];
  discount_[order][bin] = rstar;
  discount_[order][bin] /= bin + 1;
  discount_[order][bin] -= rnorm;
  norm -= rnorm;
  discount_[order][bin] /= norm;
}

// Calculate Katz discounts for each order
void NGramKatz::CalculateDiscounts() {
  for (int order = 0; order < HiOrder(); ++order) {
    double rnorm = bins_ + 1;
    rnorm *= histogram_[order][bins_];
    rnorm /= histogram_[order][0];
    for (int bin = 0; bin < bins_; ++bin)
      CalculateKatzDiscount(order, bin, rnorm);
    // counts higher than largest bin are not discounted
    discount_[order][bins_] = 1;
  }
}

// return the number of bins
int NGramKatz::GetBins() const {
  return bins_;
}

// increment histogram bin
void NGramKatz::IncrementCountBin(int order, int bin) {
  ++histogram_[order][bin];
}

// Return negative log discounted count for provided negative log count
double NGramKatz::GetDiscount(double neglogcount, int order) const {
  double discounted = neglogcount, neglogdiscount;
  if (neglogcount == StdArc::Weight::Zero().Value())  // count = 0
    return neglogcount;
  int bin = GetCountBin(neglogcount, bins_, 1);
  if (bin >= 0) {
    neglogdiscount = -log(discount_[order][bin]);
    discounted += neglogdiscount;  // multiply discount times raw count
  } else {
    LOG(FATAL) << "NGramKatz: No discount bin for discounting";
  }
  return discounted;
}

}  // namespace ngram
