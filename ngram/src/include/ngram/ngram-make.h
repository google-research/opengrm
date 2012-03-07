// ngram-make.h
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
// 
// NGram model class for making a model from raw counts

#ifndef NGRAM_NGRAM_MAKE_H__
#define NGRAM_NGRAM_MAKE_H__

#include <vector>

#include<ngram/ngram-model.h>

namespace ngram {

class NGramMake : public NGramModel {
 public:

  // Construct NGramMake object, consisting of the FST and some
  // information about the states under the assumption that the FST is a model.
  // Ownership of the FST is retained by the caller.
  NGramMake(StdMutableFst *infst, bool backoff, Label backoff_label = 0, 
	    double norm_eps = kNormEps, bool check_consistency = false)
    : NGramModel(infst, backoff_label, norm_eps, check_consistency), 
      backoff_(backoff) {}

  virtual ~NGramMake() {}

  protected:
  // Normalize n-gram counts and smooth to create an n-gram model
  void MakeNGramModel();
  
  // Return negative log discounted count for provided negative log count
  // Need to override if some discounting is done during smoothing
  // Default can be used by non-discounting methods, e.g., Witten-Bell
  virtual double GetDiscount(double nlog_count, int order) const {
    return nlog_count;
  }
  
  // Additional count mass at state if nothing reserved via smoothing method
  // Override if method requires less or more; usmoothed should be zero
  // Default can be used by most methods
  virtual double EpsilonMassIfNoneReserved() const {
    return 1;
  }
  
  // Number of bins for discounting
  // Need to override if smoothing methods keeps histogram or discount bins
  // Default can be used by non-discounting methods, e.g., Witten-Bell
  virtual int GetBins() const {
    return 0;
  }

  // Incrementing a bin of counts
  // Need to override if incrementing histogram counts for smoothing
  // Default can be used by non-discounting methods, e.g., Witten-Bell
  virtual void IncrementCountBin(int order, int bin) {
    return;
  }

  // Return high order count mass (sum of discounted counts)
  // Need to override if high order mass is not defined by discounts
  // Default can be used by discounting methods, e.g., Katz or Absolute Disc.
  virtual double CalculateHiOrderMass(vector<double> *discounts,
				      double nlog_count) {
    double discount_norm = (*discounts)[0];  // discounted count of </s>
    for (int i = 1; i < discounts->size(); ++i)  // Sum discount counts
      discount_norm = NegLogSum(discount_norm, (*discounts)[i]);
    return discount_norm;
  }

  // Return normalization constant given the count and state
  // Need to override if normalization constant is not just the count
  // Default can be used if the normalizing constant is just count
  virtual double CalculateTotalMass(double nlog_count, StateId st) {
    return nlog_count;
  }

  // Put ngram in bin = count - 1 for 0 < count <= bins
  // include big counts when discounting, but not when building histograms
  int GetCountBin(double weight_, int bins, bool includebig) const {
    int wt = int(WeightRep(weight_, 0, 1)) - 1;  // rounding count to integer
    if (wt < 0 || (!includebig && wt > bins))  // if bin should not be assigned
      wt = -1;
    else if (wt > bins)  // include big counts in highest bin discounting
      wt = bins;
    return wt;
  }

  // Find bin for the value provided and increment the histogram for that bin
  void IncrementBinCount(int order, double value) {
    int bin = GetCountBin(value, GetBins(), 0);
    if (bin >= 0)
      IncrementCountBin(order, bin);
  }

  // Calculate count histograms
  void CalculateHistograms() {
    for (StateId st = 0; st < GetFst().NumStates(); ++st) {  // get histograams
      int order = StateOrder(st) - 1;  // order starts from 0 here, not 1
      for (ArcIterator<StdExpandedFst> aiter(GetFst(), st);
	   !aiter.Done();
	   aiter.Next()) {
	StdArc arc = aiter.Value();
	if (arc.ilabel != BackoffLabel())  // no count from backoff
	  IncrementBinCount(order, arc.weight.Value());
      }
      IncrementBinCount(order, GetFst().Final(st).Value());
    }
  }
  
  // Initialize Histograms and Discounts
  void InitializeCountBins(vector < vector <double> > *countbins, int bins) {
    for (int order = 0; order < HiOrder(); ++order) {  // for each order
      vector<double> countbin;
      for (int bin = 0; bin <= bins; ++bin)  // space for bins + 1
	countbin.push_back(0);
      countbins->push_back(countbin);
    }
  }

  // Calculate smoothed value for arc out of a state
  double SmoothVal(double discount_cnt, double norm,
		   double neglog_bo_prob, double backoff_weight) const {
    double value = discount_cnt - norm;
    if (!backoff_) {
      double mixvalue = neglog_bo_prob + backoff_weight;
      value = NegLogSum(value, mixvalue);
    }
    return value;
  }

  // Calculate smoothed values for all arcs leaving a state
  void NormalizeStateArcs(StateId st, double Norm,
			  double neglog_bo_prob, vector<double> *discounts);

  // Collects discounted counts into vector, and returns normalization constant
  double CollectDiscounts(StateId st, vector<double> *discounts) const;

  // Show discounts for the given method
  void ShowDiscounts(vector < vector <double> > *discounts,
		     const char* label, int bins) const;

  // Normalize and smooth states, using parameterized smoothing method
  void SmoothState(StateId st);

  private:
  bool backoff_;  // whether to make the model as backoff or mixture model

  DISALLOW_COPY_AND_ASSIGN(NGramMake);
};


}  // namespace ngram

#endif  // NGRAM_NGRAM_MAKE_H__
