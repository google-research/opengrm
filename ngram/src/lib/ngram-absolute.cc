// ngram-absolute.cc
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

#include <ngram/ngram-absolute.h>

namespace ngram {

using std::vector;

using fst::VectorFst;
using fst::StdILabelCompare;

// Normalize n-gram counts and smooth to create an n-gram model
// Using Absolute Discounting Methods
//  'parameter': discount D
//   number of 'bins' used by Absolute Discounting (>=1)
void NGramAbsolute::MakeNGramModel() {
  if (bins_ <= 0) bins_ = 1;
  InitializeCountBins(&histogram_, bins_);
  InitializeCountBins(&discount_, bins_);
  CalculateHistograms();
  CalculateDiscounts();
  if (FLAGS_v > 0)
    ShowDiscounts(&discount_, "Absolute", bins_);
  NGramMake::MakeNGramModel();
}

// Calculate discounts for each order
void NGramAbsolute::CalculateDiscounts() {
  for (int order = 0; order < HiOrder(); ++order) {
    for (int bin = 0; bin < bins_; ++bin)
      CalculateAbsoluteDiscount(order, bin);
    // counts higher than largest bin are discounted at largest bin rate
    discount_[order][bins_] = discount_[order][bins_ - 1];
  }
}

// return the number of bins
int NGramAbsolute::GetBins() const {
  return bins_;
}

// increment histogram bin
void NGramAbsolute::IncrementCountBin(int order, int bin) {
  ++histogram_[order][bin];
}

// Return negative log discounted count for provided negative log count
double NGramAbsolute::GetDiscount(double neglogcount, int order) const {
  double discounted = neglogcount, neglogdiscount;
  if (neglogcount == StdArc::Weight::Zero().Value())  // count = 0
    return neglogcount;
  int bin = GetCountBin(neglogcount, bins_, 1);
  if (bin >= 0) {
    neglogdiscount = -log(discount_[order][bin]);
    if (neglogdiscount <= neglogcount)  // c - D <= 0
      discounted = StdArc::Weight::Zero().Value();  // set count to 0
    else
      discounted = NegLogDiff(neglogcount, neglogdiscount);  // subtract
  } else {
    LOG(FATAL) << "NGramMake: No discount bin for discounting";
  }
  return discounted;
}

}  // namespace ngram
