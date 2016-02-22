// ngram-transfer.cc
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
// Class for transferring n-grams across multiple parts split by context.

#include <map>
#include <string>
#include <vector>

#include <ngram/ngram-mutable-model.h>
#include <ngram/ngram-transfer.h>

namespace ngram {

using std::map;

StdArc::StateId NGramTransfer::FindNextState(StateId s, StdArc::Label label)
    const {
  Matcher<StdFst> matcher(*dest_fst_, MATCH_INPUT);
  matcher.SetState(s);
  StateId bs = s;
  Label find_backoff_label =
      src_model_->BackoffLabel() ? src_model_->BackoffLabel() : kNoLabel;
  bool found = false;
  do {
    if(matcher.Find(find_backoff_label))
      bs = matcher.Value().nextstate;
    else
      return bs;
    matcher.SetState(bs);
    found = matcher.Find(label);
  } while (!found);
  return matcher.Value().nextstate;
}

void NGramTransfer::TransferNGrams(bool normalize) const {
  std::vector<StateId> states(src_model_->NumStates(), kNoStateId);
  states[src_fst_->Start()] = dest_fst_->Start();
  states[src_model_->UnigramState()] = dest_model_->UnigramState();

  for (int order = 1; order <= src_model_->HiOrder(); ++order) {
    for (StateId s = 0; s < src_model_->NumStates(); ++s) {
      if (src_model_->StateOrder(s) != order) continue;
      if (states[s] == kNoStateId) continue;
      StateId sp = states[s];

      // (1) if both ascending, set states[d] = dp
      src_matcher_->SetState(s);
      for (ArcIterator<StdFst> aiter(*dest_fst_, sp);
           !aiter.Done();
           aiter.Next()) {
        const StdArc &arc = aiter.Value();
        if (arc.ilabel == dest_model_->BackoffLabel()) continue;
        if (dest_model_->StateOrder(arc.nextstate) <=
            dest_model_->StateOrder(sp)) {
          continue;
        }
        if (!src_matcher_->Find(arc.ilabel)) continue;
        if (dest_model_->StateOrder(arc.nextstate) !=
            src_model_->StateOrder(src_matcher_->Value().nextstate)) {
          continue;
        }
        CHECK_EQ(states[src_matcher_->Value().nextstate], kNoStateId);
        states[src_matcher_->Value().nextstate] = arc.nextstate;
      }

      // (2) if strictly in context in the reference, do the transfer
      if (!src_context_->HasContext(src_model_->StateNGram(s), false)) continue;

      map<Label, StdArc> arcs;
      for (ArcIterator<StdFst> aiter(*dest_fst_, sp);
           !aiter.Done();
           aiter.Next()) {
        const StdArc &arc = aiter.Value();
        arcs[arc.ilabel] = arc;
      }

      // If loosely in context for the destination, missing arcs and finality
      // need to be added.
      bool add_missing =
          dest_context_->HasContext(src_model_->StateNGram(s), true);

      for (ArcIterator<StdFst> aiter(*src_fst_, s); !aiter.Done(); aiter.Next()) {
        const StdArc &arc = aiter.Value();
        map<Label, StdArc>::iterator iter = arcs.find(arc.ilabel);
        if (iter != arcs.end()) {
          iter->second.weight = arc.weight;
        } else if (add_missing) {
          arcs[arc.ilabel] =
              StdArc(arc.ilabel, arc.olabel, arc.weight,
                     FindNextState(sp, arc.ilabel));
        }
      }

      dest_fst_->DeleteArcs(sp);
      for (map<Label, StdArc>::const_iterator iter = arcs.begin();
           iter != arcs.end(); ++iter) {
        dest_fst_->AddArc(sp, iter->second);
      }
      if (src_fst_->Final(s) != Weight::Zero() &&
          (add_missing || dest_fst_->Final(sp) != Weight::Zero())) {
        dest_fst_->SetFinal(sp, src_fst_->Final(s));
      }

      if (normalize)
        dest_model_->RecalcBackoff(sp);
    }
  }
}

}  // namespace ngram
