// ngram-split.cc
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
// Class for splitting an NGram model into multiple parts by context.

#include <sstream>
#include <fst/arcsort.h>
#include <ngram/ngram-context.h>
#include <ngram/ngram-split.h>

namespace ngram {

// Split based on context patterns (sse ngram-context.h for more
// information).
NGramSplit::NGramSplit(const StdFst &fst,
                       const vector<string> &context_patterns,
                       StdArc::Label backoff_label,
                       double norm_eps)
    : model_(fst, backoff_label, norm_eps, true),
      split_(0) {
  for (int i = 0; i < context_patterns.size(); ++i) {
    contexts_.push_back(
        new NGramContext(context_patterns[i], model_.HiOrder()));
    fsts_.push_back(new StdVectorFst);
    fsts_.back()->SetInputSymbols(fst.InputSymbols());
    fsts_.back()->SetOutputSymbols(fst.OutputSymbols());
  }
  state_maps_.resize(contexts_.size());
  state_splits_.resize(model_.NumStates());
  SplitNGramModel(fst);
  for (int i = 0; i < fsts_.size(); ++i)
    ArcSort(fsts_[i], fst::StdILabelCompare());
}


// Split based on context begin and end vectors (sse ngram-context.h
// for more information).
NGramSplit::NGramSplit(const StdFst &fst,
                       const vector< vector<Label> > &contexts,
                       StdArc::Label backoff_label,
                       double norm_eps)
    : model_(fst, backoff_label, norm_eps, true),
      split_(0) {
  for (int i = 0; i < contexts.size() - 1; ++i) {
    contexts_.push_back(
        new NGramContext(contexts[i], contexts[i + 1], model_.HiOrder()));
    fsts_.push_back(new StdVectorFst);
  }
  SplitNGramModel(fst);
  for (int i = 0; i < fsts_.size(); ++i)
    ArcSort(fsts_[i], fst::StdILabelCompare());
}

void NGramSplit::SplitNGramModel(const StdFst &fst) {
  // For each state, compute the strict split it belongs to.
  for (StateId state = 0; state < model_.NumStates(); ++state) {
    const vector<Label> &ngram = model_.StateNGram(state);
    for (size_t i = 0; i < contexts_.size(); ++i) {
      if (contexts_[i]->HasContext(ngram, false))
        state_splits_[state].insert(i);
    }
  }

  // Make sure the start state is added to every split.
  for (size_t i = 0; i < contexts_.size(); ++i)
    state_splits_[fst.Start()].insert(i);

  // For each state, compute the sets of splits it needs to belong
  // to maintain the core model topology: needed ascending arcs and
  // backoff arcs.
  for (int order = model_.HiOrder(); order > 0; --order) {
    for (StateId state = 0; state < model_.NumStates(); ++state) {
      if (model_.StateOrder(state) != order) continue;
      // Second get more splits from ascending arcs.
      for (ArcIterator<StdFst> aiter(fst, state); !aiter.Done(); aiter.Next()) {
        const StdArc &arc = aiter.Value();
        if (model_.StateOrder(arc.nextstate) != model_.StateOrder(state) + 1)
          continue;
        for (set<size_t>::const_iterator iter =
                 state_splits_[arc.nextstate].begin();
             iter != state_splits_[arc.nextstate].end(); ++iter) {
          state_splits_[state].insert(*iter);
        }
      }
      // TODO(allauzen): decide whether keeping backoff of out of context
      // states. If so, process backoff after ascending arcs.
      // First propage the current splits for 'state' to its backoff.
      // TODO(allauzen): rewrite to use GetBackoff.
      for (ArcIterator<StdFst> aiter(fst, state); !aiter.Done(); aiter.Next()) {
        const StdArc &arc = aiter.Value();
        if (arc.ilabel != model_.BackoffLabel()) continue;
        for (set<size_t>::const_iterator iter = state_splits_[state].begin();
             iter != state_splits_[state].end(); ++iter) {
          state_splits_[arc.nextstate].insert(*iter);
        }
      }
    }
  }

  // Make sure the unigram state was added to every split.
  for (size_t i = 0; i < contexts_.size(); ++i) {
    CHECK_EQ(state_splits_[model_.UnigramState()].count(i), 1);
  }

  // Create all required states in each split, and compute
  // state map for each split.
  for (StateId state = 0; state < model_.NumStates(); ++state) {
    for (set<size_t>::const_iterator iter = state_splits_[state].begin();
         iter != state_splits_[state].end(); ++iter) {
      state_maps_[*iter][state] = fsts_[*iter]->AddState();
    }
  }

  // Add all needed and strictly in context transitions to each split.
  for (StateId state = 0; state < model_.NumStates(); ++state) {
    for (set<size_t>::const_iterator iter = state_splits_[state].begin();
         iter != state_splits_[state].end(); ++iter) {
      size_t i = *iter;
      double bo_weight = 0.0;
      StateId bo_state = model_.GetBackoff(state, &bo_weight);

      if (bo_state != kNoStateId) {
        // Add backoff arc
        fsts_[i]->AddArc(state_maps_[i][state],
                         StdArc(0, 0, bo_weight, state_maps_[i][bo_state]));
      }

      if (state == fst.Start())
        fsts_[i]->SetStart(state_maps_[i][state]);

      const vector<Label> &ngram = model_.StateNGram(state);
      bool in_context = contexts_[i]->HasContext(ngram, false);

      for (ArcIterator<StdFst> aiter(fst, state);
           !aiter.Done(); aiter.Next()) {
        const StdArc &arc = aiter.Value();
        if (arc.ilabel == model_.BackoffLabel()) continue;
        StateId nextstate = arc.nextstate;
        if (!in_context &&
            ((model_.StateOrder(nextstate) != model_.StateOrder(state) + 1) ||
             !state_splits_[nextstate].count(i))) {
          continue;
        }
        map<StateId, StateId>::const_iterator iter =
            state_maps_[i].find(nextstate);
        CHECK(in_context || (state == fst.Start()) ||
              (iter != state_maps_[i].end()));
        while (iter == state_maps_[i].end()) {
          nextstate = model_.GetBackoff(nextstate, 0);
          CHECK_NE(nextstate, kNoStateId);
          iter = state_maps_[i].find(nextstate);
        }
        fsts_[i]->AddArc(
            state_maps_[i][state],
            StdArc(arc.ilabel, arc.olabel, arc.weight, iter->second));
      }

      if (in_context && (fst.Final(state) != Weight::Zero()))
        fsts_[i]->SetFinal(state_maps_[i][state], fst.Final(state));
    }
  }
}

}  // namespace ngram
