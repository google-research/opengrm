// ngram-make.cc
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
// NGram model class for making a model from raw counts

#include <vector>

#include <fst/arcsort.h>
#include <fst/vector-fst.h>

#include <ngram/ngram-make.h>

namespace ngram {

using std::vector;

using fst::VectorFst;
using fst::StdILabelCompare;

// Normalize n-gram counts and smooth to create an n-gram model
void NGramMake::MakeNGramModel() {
  for (StateId st = 0; st < GetExpandedFst().NumStates(); ++st)
    has_all_ngrams_.push_back(false);
  for (int order = 1; order <= HiOrder(); ++order) {  // for each order
    for (StateId st = 0; st < GetExpandedFst().NumStates(); ++st)  // and state
      if (StateOrder(st) == order)  // if state is the current order
	SmoothState(st);  // smooth it
  }
  InitModel();      // Recalculate state info
  RecalcBackoff();  // Recalculate the backoff costs
  if (!CheckNormalization())  // ensure model is fully normalized
    LOG(FATAL) << "NGramMake: Final model not fully normalized";
}

// Calculate smoothed values for all arcs leaving a state
void NGramMake::NormalizeStateArcs(StateId st, double norm,
				   double neglog_bo_prob,
				   const vector<double> &discounts) {
  StateId bo = GetBackoff(st, 0);
  if (GetFst().Final(st).Value() != StdArc::Weight::Zero().Value()) {
    GetMutableFst()->SetFinal(st, SmoothVal(discounts[0], norm,
					    neglog_bo_prob,
					    GetFst().Final(bo).Value()));
  }
  vector<double> bo_arc_weight;
  FillBackoffArcWeights(st, bo, &bo_arc_weight);  // fill backoff weight vector
  int arc_counter = 0;  // index into backoff weights
  int discount_index = 1;  // index into discounts (off by one, for </s>)
  for (MutableArcIterator<StdMutableFst> aiter(GetMutableFst(), st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel != BackoffLabel()) {  // backoff weights calculated later
      arc.weight = SmoothVal(discounts[discount_index++], norm,
			     neglog_bo_prob, bo_arc_weight[arc_counter++]);
      aiter.SetValue(arc);
    }
  }
}

// Collects discounted counts into vector, and returns -log(sum(counts))
// If no discounting, vector collects undiscounted counts
double NGramMake::CollectDiscounts(StateId st,
				   vector<double> *discounts) const {
  double nlog_count_sum = GetFst().Final(st).Value(), KahanVal = 0;
  int order = StateOrder(st) - 1;  // for retrieving discount parameters
  discounts->push_back(GetDiscount(GetFst().Final(st).Value(), order));
  for (ArcIterator<StdExpandedFst> aiter(GetExpandedFst(), st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel != BackoffLabel()) {  // skip backoff arc
      nlog_count_sum =
	NegLogSum(nlog_count_sum, arc.weight.Value(), &KahanVal);
      discounts->push_back(GetDiscount(arc.weight.Value(), order));
    }
  }
  return nlog_count_sum;
}

// Normalize and smooth states, using parameterized smoothing method
void NGramMake::SmoothState(StateId st) {
  vector<double> discounts;  // collect discounted counts for later use.
  double nlog_count_sum = CollectDiscounts(st, &discounts), nlog_stored_sum;
  if (GetBackoff(st, &nlog_stored_sum) < 0) {
    has_all_ngrams_[st] = true;
    ScaleStateWeight(st, -nlog_count_sum);  // no backoff arc, unsmoothed
  } else {
    // Calculate total count mass and higher order count mass to normalize
    double total_mass = CalculateTotalMass(nlog_stored_sum, st);
    double hi_order_mass = CalculateHiOrderMass(discounts, nlog_stored_sum);
    has_all_ngrams_[st] = HasAllArcsInBackoff(st);
    if (has_all_ngrams_[st] && total_mass < hi_order_mass) {
      discounts[0] = 
	NegLogSum(discounts[0], NegLogDiff(total_mass, hi_order_mass));
      hi_order_mass = total_mass;
    }
    double low_order_mass;
    if (total_mass >= hi_order_mass &&  // if approx equal
	fabs(total_mass - hi_order_mass) < kFloatEps)
      total_mass = hi_order_mass;  // then make equal, for later testing
    if (has_all_ngrams_[st] || (total_mass == hi_order_mass && 
				EpsilonMassIfNoneReserved() <= 0)) {
      low_order_mass = kInfBackoff;
    } else {
      if (total_mass == hi_order_mass)  // if no mass reserved, add epsilon
	total_mass = -log(exp(-total_mass) + EpsilonMassIfNoneReserved());
      low_order_mass = NegLogDiff(total_mass, hi_order_mass);
    }
    NormalizeStateArcs(st, total_mass, low_order_mass - total_mass,
		       discounts);
  }
}

// Checks to see if all n-grams already represented at state
bool NGramMake::HasAllArcsInBackoff(StateId st) {
  StateId bo = GetBackoff(st, 0);
  if (!has_all_ngrams_[bo]) return false;  // backoff state doesn't have all
  size_t starcs = GetFst().NumArcs(st), boarcs = GetFst().NumArcs(bo);
  if (boarcs > starcs) return false;  // arcs at backoff not in current state
  if (GetFst().Final(bo) != StdArc::Weight::Zero())  // count </s> symbol
    boarcs++;
  if (GetBackoff(bo, 0) >= 0) boarcs--;  // don't count backoff arc
  if (GetFst().Final(st) != StdArc::Weight::Zero())  // count </s> symbol
    starcs++;
  starcs--;  // don't count backoff arc
  if (boarcs == starcs) return true;
  return false;
}

}  // namespace ngram
