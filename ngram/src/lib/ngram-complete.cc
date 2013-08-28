// ngram-complete.cc
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
// Complete a partial model by adding transitions to ensure proper topology
// as required by NGramModel.

#include <algorithm>
#include <set>
#include <vector>

#include <fst/arcsort.h>
#include <fst/fst.h>
#include <fst/matcher.h>
#include <fst/mutable-fst.h>

#include <ngram/ngram-complete.h>

namespace ngram {

using fst::ArcIterator;
using fst::kNoLabel;
using fst::kNoStateId;
using fst::Matcher;
using fst::MATCH_INPUT;
using fst::StdArc;
using fst::StdILabelCompare;
using fst::StdMutableFst;
using fst::StdFst;

using std::set;

void NGramComplete(StdMutableFst *fst, StdArc::Label backoff_label) {
  vector<int> state_orders;
  StdArc::StateId unigram_state = kNoStateId;

  Matcher<StdFst> *matcher = new Matcher<StdFst>(*fst, MATCH_INPUT);
  CHECK_EQ(matcher->Type(true), MATCH_INPUT);
  for (StdArc::StateId s = 0; s < fst->NumStates(); ++s) {
    matcher->SetState(s);
    matcher->Find(backoff_label ? backoff_label : kNoLabel);
    if (matcher->Done()) {
      state_orders.push_back(1);
      CHECK_EQ(unigram_state, kNoStateId);
      unigram_state = s;
      VLOG(1) << "Unigram state: " << unigram_state;
      continue;
    }
    StdArc::StateId bs = matcher->Value().nextstate;
    int order = 1;
    while (bs >= state_orders.size()) {
      matcher->SetState(bs);
      if (!matcher->Find(backoff_label ? backoff_label : kNoLabel))
        break;
      bs = matcher->Value().nextstate;
      order++;
    }
    if (bs < state_orders.size())
      order += state_orders[bs];
    else
      ++order;
    state_orders.push_back(order);
  }

  {
    vector<int> order_counts;
    vector<int> ngram_counts;
    for (ssize_t s = 0; s < fst->NumStates(); ++s) {
      while (order_counts.size() <= state_orders[s])
        order_counts.push_back(0);
      ++order_counts[state_orders[s]];
      while (ngram_counts.size() <= state_orders[s])
        ngram_counts.push_back(0);
      int ngrams = fst->NumArcs(s)  - 1;
      if (fst->Final(s) != StdArc::Weight::Zero())
        ++ngrams;
      ngram_counts[state_orders[s]] += ngrams;
    }

    CHECK_EQ(order_counts[0], 0);
    CHECK_EQ(ngram_counts[0], 0);
    for (int i = 1; i < order_counts.size(); ++i)
      VLOG(1) << "# states at order " << i << " : " << order_counts[i];
    for (int i = 1; i < ngram_counts.size(); ++i)
      VLOG(1) << "# n-grams at order " << i << " : " << ngram_counts[i];

    VLOG(1) << "Order of unigram state " << unigram_state << " : "
              << state_orders[unigram_state];
    VLOG(1) << "Order of start state " << fst->Start() << " : "
              << state_orders[fst->Start()];
  }


  int hi_order = *max_element(state_orders.begin(), state_orders.end());
  vector<set<StdArc::Label> > label_sets(fst->NumStates());
  set<StdArc::StateId> new_final_states;

  for (int order = hi_order; order > 1; --order) {
    for (StdArc::StateId s = 0; s < fst->NumStates(); ++s) {
      if (state_orders[s] != order) continue;

      matcher->SetState(s);
      CHECK(matcher->Find(backoff_label ? backoff_label : kNoLabel));
      StdArc::StateId bs = matcher->Value().nextstate;
      CHECK_EQ(state_orders[s], state_orders[bs] + 1);

      matcher->SetState(bs);
      for (ArcIterator<StdFst> aiter(*fst, s); !aiter.Done(); aiter.Next()) {
        const StdArc &arc = aiter.Value();
        if (arc.ilabel == backoff_label) continue;
        if (matcher->Find(arc.ilabel)) continue;
        label_sets[bs].insert(arc.ilabel);
      }

      for (set<StdArc::Label>::const_iterator iter =
             label_sets[s].begin();
           iter != label_sets[s].end(); ++iter) {
        if (matcher->Find(*iter)) continue;
        label_sets[bs].insert(*iter);
      }

      if ((fst->Final(s) != StdArc::Weight::Zero() ||
           new_final_states.count(s) != 0) &&
          fst->Final(bs) == StdArc::Weight::Zero()) {
        new_final_states.insert(bs);
      }
    }
  }

  delete matcher;

  for (int order = 1; order < hi_order; ++order) {
    for (StdArc::StateId s = 0; s < fst->NumStates(); ++s) {
      if (state_orders[s] != order) continue;

      Matcher<StdFst> *updated_matcher = new Matcher<StdFst>(*fst, MATCH_INPUT);
      updated_matcher->SetState(s);
      StdArc::StateId bs = kNoStateId;
      if (updated_matcher->Find(backoff_label ? backoff_label : kNoLabel)) {
        bs = updated_matcher->Value().nextstate;
        updated_matcher->SetState(bs);
      }

      vector<StdArc> arcs;
      arcs.reserve(fst->NumArcs(s) + label_sets[s].size());

      for (set<StdArc::Label>::const_iterator iter =
             label_sets[s].begin();
           iter != label_sets[s].end(); ++iter) {
        StdArc::StateId nextstate = unigram_state;
        if (bs != kNoStateId && updated_matcher->Find(*iter))
          nextstate = updated_matcher->Value().nextstate;
        arcs.push_back(StdArc(*iter, *iter, StdArc::Weight::One(), nextstate));
      }

      delete updated_matcher;

      for (ArcIterator<StdFst> aiter(*fst, s); !aiter.Done(); aiter.Next())
        arcs.push_back(aiter.Value());

      fst->DeleteArcs(s);
      sort(arcs.begin(), arcs.end(), StdILabelCompare());

      for (size_t i = 0; i < arcs.size(); ++i)
        fst->AddArc(s, arcs[i]);
    }
  }

  for (set<StdArc::StateId>::const_iterator iter = new_final_states.begin();
       iter != new_final_states.end(); ++iter) {
    fst->SetFinal(*iter, StdArc::Weight::One());
  }


  if (fst->Final(unigram_state) == StdArc::Weight::Zero())
    fst->SetFinal(unigram_state, StdArc::Weight::One());
}

}  // namespace ngram
