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
// Copyright 2009-2011 Brian Roark and Google, Inc.
// Authors: roarkbr@gmail.com  (Brian Roark)
//          allauzen@google.com (Cyril Allauzen)
//          riley@google.com (Michael Riley)
//
// \file

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
  for (int order = 1; order <= HiOrder(); ++order) {  // for each order
    for (StateId st = 0; st < GetFst().NumStates(); ++st)  // scan states
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
				   vector<double> *discounts) {
  StateId bo = GetBackoff(st, 0);
  if (GetFst().Final(st).Value() != StdArc::Weight::Zero().Value()) {
    GetMutableFst()->SetFinal(st, SmoothVal((*discounts)[0], norm,
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
      arc.weight = SmoothVal((*discounts)[discount_index++], norm,
			     neglog_bo_prob, bo_arc_weight[arc_counter++]);
      aiter.SetValue(arc);
    }
  }
}

// Collects discounted counts into vector, and returns -log(sum(counts))
// If no discounting, vector collects undiscounted counts
double NGramMake::CollectDiscounts(StateId st, 
				   vector<double> *discounts) const {
  double nlog_count_sum = GetFst().Final(st).Value();
  int order = StateOrder(st) - 1;  // for retrieving discount parameters
  discounts->push_back(GetDiscount(GetFst().Final(st).Value(), order));
  for (ArcIterator<StdExpandedFst> aiter(GetFst(), st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel != BackoffLabel()) {  // skip backoff arc
      nlog_count_sum = NegLogSum(nlog_count_sum, arc.weight.Value());
      discounts->push_back(GetDiscount(arc.weight.Value(), order));
    }
  }
  return nlog_count_sum;
}

// Normalize and smooth states, using parameterized smoothing method
void NGramMake::SmoothState(StateId st) {
  vector<double> discounts;  // collect discounted counts for later use.
  double nlog_count_sum = CollectDiscounts(st, &discounts);
  if (GetBackoff(st, 0) < 0) {
    ScaleStateWeight(st, -nlog_count_sum);  // no backoff arc, unsmoothed
  } else {
    // Calculate total count mass and higher order count mass to normalize
    double total_mass = CalculateTotalMass(nlog_count_sum, st);
    double hi_order_mass = CalculateHiOrderMass(&discounts, nlog_count_sum);
    double low_order_mass;
    if (fabs(total_mass - hi_order_mass) < NormEps())  // if approx equal
      total_mass = hi_order_mass;  // then make equal, for later testing
    if (total_mass == hi_order_mass && EpsilonMassIfNoneReserved() <= 0) {
      low_order_mass = StdArc::Weight::Zero().Value();
    } else {
      if (total_mass == hi_order_mass)  // if no mass reserved, add epsilon
	total_mass = -log(exp(-total_mass) + EpsilonMassIfNoneReserved());
      low_order_mass = NegLogDiff(total_mass, hi_order_mass);
    }
    NormalizeStateArcs(st, total_mass,
		       low_order_mass - total_mass, &discounts);
  }
}

// Display discount values to cerr if requested
void NGramMake::ShowDiscounts(vector < vector <double> > *discounts,
			      const char *label, int bins) const {
  cerr << "Count bin   ";
  cerr << label; 
  cerr << " Discounts (";
  for (int order = 0; order < HiOrder(); ++order) {
    if (order > 0) cerr << "/";
    cerr << order + 1 << "-grams";
  }
  cerr << ")" << endl;
  for (int bin = 0; bin <= bins; ++bin) {
    if (bin < bins)
      cerr << "Count = " << bin + 1 << "   ";
    else
      cerr << "Count > " << bin  << "   ";
    for (int order = 0; order < HiOrder(); ++order) {
      if (order > 0) cerr << "/";
      cerr << (*discounts)[order][bin];
    }
    cerr << endl;
  }
}

}  // namespace ngram
