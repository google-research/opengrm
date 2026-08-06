
// Licensed under the Apache License, Version 2.0 (the 'License');
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an 'AS IS' BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2018 Google, Inc.
// canonical.h
//
// Tests if stochastic FSTs have canonical topology wrt any failure
// (and/or epsilon) transitions.

#ifndef SFST_CANONICAL_H_
#define SFST_CANONICAL_H_

#include <vector>

#include <fst/log.h>
#include <fst/dfs-visit.h>
#include <fst/fst.h>
#include <fst/matcher.h>
#include <fst/topsort.h>
#include <fst/vector-fst.h>

namespace sfst {

// Finds the topological ordering of the states w.r.t. the (input) phi-labeled
// (and epsilon) transitions. If acyclic w.r.t. these transitions,
// then top_order[i] gives the ith state ID in the phi-topological order
// and true is returned, else top_order is unchanged and false is returned.
template <class Arc>
bool PhiTopOrder(const fst::Fst<Arc> &fst,
                 typename Arc::Label phi_label,
                 std::vector<typename Arc::StateId> *top_order) {
  namespace f = fst;
  typedef typename Arc::StateId StateId;

  f::MultiLabelArcFilter<Arc> label_filter;
  if (phi_label != f::kNoLabel)
    label_filter.AddLabel(phi_label);
  if (phi_label != 0)
    label_filter.AddLabel(0);
  bool acyclic;
  std::vector<StateId> inv_order;
  f::TopOrderVisitor<Arc> top_order_visitor(&inv_order, &acyclic);
  f::DfsVisit(fst, &top_order_visitor, label_filter);
  if (acyclic) {
    top_order->resize(inv_order.size());
    for (StateId s = 0; s < inv_order.size(); ++s) {
      (*top_order)[inv_order[s]] = s;
    }
  }
  return acyclic;
}

// CANONICAL SFST TOPOLOGY: (input) arc-sorted and if (input) phi
// labeled, at most one phi-labeled transition per state and no
// phi-labeled cycles.  No assumption is made of general determinism
// or what transitions must be present on failure (unlike in a
// canonical n-gram model).

// Tests if input has the canonical topology of a stochastic FST. This
// version is passed the canonical top_order vector to fill in (as
// returned by PhiTopOrder).
template <class Arc>
bool IsCanonical(const fst::Fst<Arc> &fst, typename Arc::Label phi_label,
                 std::vector<typename Arc::StateId> *top_order) {
  namespace f = fst;
  typedef typename Arc::StateId StateId;
  typedef f::ExplicitMatcher<f::Matcher<f::Fst<Arc>>> Matr;
  typedef f::StateIterator<f::Fst<Arc>> StateItr;

  bool phi_acyclic = PhiTopOrder(fst, phi_label, top_order);
  // phi label cycle?
  if (!phi_acyclic) {
    VLOG(1) << "IsCanonical: input FST not acyclic w.r.t. phi/epsilon labels";
    return false;
  }
  typedef typename Arc::StateId StateId;
  // input-label sorted?
  if (!fst.Properties(f::kILabelSorted, true)) {
    VLOG(1) << "IsCanonical: input FST not sorted w.r.t input labels";
    return false;
  }

  if (phi_label != f::kNoLabel) {
    // At most one phi-label per state?
    Matr matcher(fst, f::MATCH_INPUT);
    for (StateItr siter(fst); !siter.Done(); siter.Next()) {
      StateId s = siter.Value();
      matcher.SetState(s);
      if (matcher.Find(phi_label)) {
        matcher.Next();
        if (!matcher.Done()) {
          VLOG(1) << "IsCanonical: phi non-determinism: state: " << s;
          return false;
        }
      }
    }
  }

  return true;
}

// Tests if input has the canonical topology of a stochastic FST.
template <class Arc>
bool IsCanonical(const fst::Fst<Arc> &fst,
                 typename Arc::Label phi_label = fst::kNoLabel) {
  typedef typename Arc::StateId StateId;
  std::vector<StateId> top_order;
  return IsCanonical(fst, phi_label, &top_order);
}

// Finds the phi order of each state: i.e., the length of the (longest)
// phi path from each state + 1. Assumes but does not fully test that
// the input is a canonical SFST. Returns the highest order found.
template <class Arc>
int PhiStateOrder(const fst::Fst<Arc> &fst, typename Arc::Label phi_label,
                   std::vector<int> *state_order) {
  namespace f = fst;
  typedef typename Arc::StateId StateId;
  typedef f::ExplicitMatcher<f::Matcher<f::Fst<Arc>>> Matr;
  state_order->clear();
  if (phi_label == f::kNoLabel) {
    state_order->resize(f::CountStates(fst), 1);
    return 1;
  }
  std::vector<StateId> top_order;
  bool acyclic = PhiTopOrder(fst, phi_label, &top_order);
  if (!acyclic) {
    FSTERROR() << "PhiStateOrder: input FST not canonical (phi-cyclic)";
    return 0;
  }
  state_order->resize(top_order.size(), 1);
  int max_order = 1;

  Matr matcher(fst, f::MATCH_INPUT);
  for (StateId i = top_order.size() - 1; i >= 0; --i) {
    StateId s = top_order[i];   // ith state in reverse phi-top order
    matcher.SetState(s);
    if (matcher.Find(phi_label)) {
      const Arc &arc = matcher.Value();
      int ord = (*state_order)[arc.nextstate] + 1;
      (*state_order)[s] = ord;
      if (ord > max_order)
        max_order = ord;
    }
  }
  return max_order;
}


}  // namespace sfst

#endif  // SFST_CANONICAL_H_
