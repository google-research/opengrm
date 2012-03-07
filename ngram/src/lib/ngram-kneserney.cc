// ngram-kneserney.cc
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

#include <ngram/ngram-kneserney.h>

namespace ngram {

using std::vector;

using fst::VectorFst;
using fst::StdILabelCompare;

// Normalize n-gram counts and smooth to create an n-gram model 
// Using Kneser-Ney methods.
//  'parameter' is discount D for AD
//   number of 'bins' used by Absolute Discounting (>=1)
void NGramKneserNey::MakeNGramModel() {
  AssignKneserNeyCounts();  // Perform KN marginalization of lower orders
  // Then build Absolute Discounting model
  NGramAbsolute::MakeNGramModel();
}

// Update arc and final values, either initializing or incrementing
void NGramKneserNey::UpdateKneserNeyCounts(StateId st, bool increment) {
  StateId bo = GetBackoff(st, 0);
  if (bo >= 0) {  // if bigram or higher order
    MutableArcIterator<StdMutableFst> biter(GetMutableFst(), bo);
    for (ArcIterator<StdExpandedFst> aiter(GetFst(), st);
	 !aiter.Done();
	 aiter.Next()) {
      StdArc arc = aiter.Value();
      if (arc.ilabel == BackoffLabel())
	continue;
      if (FindMutableArc(&biter, arc.ilabel)) {
	StdArc barc = biter.Value();
	if (increment) {  // incrementing the counts
	  barc.weight = NegLogSum(barc.weight.Value(), 0);
	} else {  // initializing counts to zero
	  barc.weight = StdArc::Weight::Zero().Value();
	}
	biter.SetValue(barc);
      } else {
	LOG(FATAL) << "NGramMake:  lower order arc missing";
      }
    }
    // No </s> arc, so final state if non-zero probability
    if (GetFst().Final(st).Value() != StdArc::Weight::Zero().Value()) {
      if (increment) {  // incrementing the counts
	GetMutableFst()->SetFinal(bo,
				  NegLogSum(GetFst().Final(bo).Value(), 0));
      } else {  // initializing counts to zero
	GetMutableFst()->SetFinal(bo, StdArc::Weight::Zero().Value());
      }
    }
  }
}

// Modify lower order counts according to Kneser Ney formula
void NGramKneserNey::AssignKneserNeyCounts() {
  // Initialize values starting from lowest order to highest
  // Must be in ascending order because of final cost initialization
  for (int order = 2; order <= HiOrder(); ++order) {  // for each order
    for (StateId st = 0; st < GetFst().NumStates(); ++st)  // scan states
      if (StateOrder(st) == order)  // if state is the current order
	UpdateKneserNeyCounts(st, 0);
  }
  // Increment values starting from highest order to lowest
  // Must be in descending order because of final cost incrementing
  for (int order = HiOrder(); order > 1; --order) {  // for each order
    for (StateId st = 0; st < GetFst().NumStates(); ++st)  // scan states
      if (StateOrder(st) == order)  // if state is the current order
	UpdateKneserNeyCounts(st, 1);
  }
}

}  // namespace ngram
