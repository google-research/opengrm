// ngram-mutable-model.cc
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
// NGram mutable model class

#include <deque>

#include <fst/arcsort.h>
#include <fst/vector-fst.h>

#include <ngram/ngram-mutable-model.h>

namespace ngram {

using std::deque;

using fst::VectorFst;
using fst::StdILabelCompare;

using fst::kAcceptor;
using fst::kIDeterministic;
using fst::kILabelSorted;

// Calculate and assign backoff cost from neglog sums of hi and low order arcs
void NGramMutableModel::UpdateBackoffCost(StateId st, double hi_neglog_sum,
					  double low_neglog_sum) {
  double alpha = CalculateBackoffCost(hi_neglog_sum, low_neglog_sum, 
				      infinite_backoff_);
  AdjustCompleteStates(st, &alpha);
  MutableArcIterator<StdMutableFst> aiter(mutable_fst_, st);
  if (FindMutableArc(&aiter, BackoffLabel())) {
    StdArc arc = aiter.Value();
    arc.weight = alpha;
    aiter.SetValue(arc);
  } else {
    LOG(FATAL) << "NGramMutableModel: No backoff arc found: " << st;
  }
}

// Sets alpha to kInfBackoff for states with every possible n-gram
void NGramMutableModel::AdjustCompleteStates(StateId st, double *alpha) {
  int unigram_state = UnigramState();
  if (unigram_state < 0) unigram_state = GetFst().Start();
  if (NumNGrams(unigram_state) == NumNGrams(st))
    (*alpha) = kInfBackoff;
}

// Scale weights by some factor, for normalizing and use in model merging
void NGramMutableModel::ScaleStateWeight(StateId st, double scale) {
  if (mutable_fst_->Final(st) != StdArc::Weight::Zero())
    mutable_fst_->SetFinal(st, Times(mutable_fst_->Final(st), scale));
  for (MutableArcIterator<StdMutableFst> aiter(mutable_fst_, st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel != BackoffLabel()) {  // only scaling non-backoff arcs
      arc.weight = Times(arc.weight, scale);
      aiter.SetValue(arc);
    }
  }
}

// Sorts arcs in state in ilabel order.
void NGramMutableModel::SortArcs(StateId s) {
  StdILabelCompare comp;
  vector<StdArc> arcs;
  for (ArcIterator<StdMutableFst> aiter(*mutable_fst_, s);
       !aiter.Done();
       aiter.Next())
    arcs.push_back(aiter.Value());
  sort(arcs.begin(), arcs.end(), comp);
  mutable_fst_->DeleteArcs(s);
  for (size_t a = 0; a < arcs.size(); ++a)
    mutable_fst_->AddArc(s, arcs[a]);
}

// Scan arcs and remove lower order from arc weight
void NGramMutableModel::UnSumState(StateId st) {
  double bocost;
  StateId bo = GetBackoff(st, &bocost);
  for (MutableArcIterator<StdMutableFst> aiter(mutable_fst_, st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel == BackoffLabel())
      continue;
    arc.weight = NegLogDiff(arc.weight.Value(),
			    FindArcWeight(bo, arc.ilabel) + bocost);
    aiter.SetValue(arc);
  }
  if (mutable_fst_->Final(st).Value() != StdArc::Weight::Zero().Value())
    mutable_fst_->SetFinal(st, NegLogDiff(mutable_fst_->Final(st).Value(),
				  mutable_fst_->Final(bo).Value() + bocost));
}

// Replace backoff weight with -log p(backoff)
void NGramMutableModel::DeBackoffNGramModel() {
  for (StateId st = 0; st < mutable_fst_->NumStates(); ++st) {  // scan states
    double hi_neglog_sum, low_neglog_sum;
    if (CalcBONegLogSums(st, &hi_neglog_sum, &low_neglog_sum)) {
      MutableArcIterator<StdMutableFst> aiter(mutable_fst_, st);
      if (FindMutableArc(&aiter, BackoffLabel())) {
        StdArc arc = aiter.Value();
        arc.weight = -log(1 - exp(-hi_neglog_sum));
        aiter.SetValue(arc);
      }
      else {
        LOG(FATAL) << "NGramMutableModel: No backoff arc found: "  << st;
      }
    }
  }
}

}  // namespace ngram
