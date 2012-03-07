// ngram-shrink.cc
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
// TODO: file description & doc.

#include <sstream>
#include <ngram/ngram-shrink.h>

namespace ngram {

using std::stringstream;

// Construct an NGramShrink object, including an NGramModel and parameters
NGramShrink::NGramShrink(StdMutableFst *infst, int shrink_opt,
			 double tot_uni, Label backoff_label,
                         double norm_eps, bool check_consistency)
    : NGramModel(infst, backoff_label, norm_eps, check_consistency),
      normalized_(CheckNormalization()),
      shrink_opt_(shrink_opt),
      total_unigram_count_(tot_uni),
      ns_(infst->NumStates()),
      dead_state_(GetMutableFst()->AddState()) {
  for (StateId st = 0; st < ns_; ++st)
    shrink_state_.push_back(ShrinkStateStats());
}

// Shrink n-gram model, based on initialized parameters
void NGramShrink::ShrinkNGramModel(bool require_norm) {
  if (normalized_) {  // only required for normalized models
    CalculateStateProbs();  // calculate p(h)
    if (total_unigram_count_ <= 0)  // auto derive unigram count if req'd
      total_unigram_count_ = EstimateTotalUnigramCount();
  } else if (require_norm) {
    LOG(FATAL) << "NGramShrink: Model not normalized;"
	       << " Model must be normalized for this shrinking method";
  }
  FillShrinkStateInfo();  // collect state information
  PruneModel();  // prune arcs and point to unconnected state
  PointArcsAwayFromDead();  // point unpruned arcs to connected states
  Connect(GetMutableFst());  // removes pruned arcs and dead states
  InitModel();        // re-calc state info
  if (normalized_) {  // only needed for normalized models
    RecalcBackoff();  // re-calc backoff weights
    if (!CheckNormalization())  // model should be normalized
      LOG(FATAL) << "NGramShrink: Pruned model not fully normalized";
  }
}

// Estimate total unigram count based on probabilities in unigram state
// The difference between two smallest probs should be 1/N, return reciprocal
double NGramShrink::EstimateTotalUnigramCount() const {
  StateId st = UnigramState();
  bool first = true;
  double max = LogArc::Weight::Zero().Value(), nextmax = max;
  if (st < 0) st = GetFst().Start();  // if model unigram, use Start()
  for (ArcIterator<StdExpandedFst> aiter(GetFst(), st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel == BackoffLabel()) continue;
    if (first || arc.weight.Value() > max) {  // maximum negative log prob
      nextmax = max;  // keep both max and nextmax (to calculate diff)
      max = arc.weight.Value();
      first = false;
    }
    else if (arc.weight.Value() < max && arc.weight.Value() > nextmax) {
      nextmax = arc.weight.Value();
    }
  }
  if (nextmax == LogArc::Weight::Zero().Value()) return exp(max);
  return exp(NegLogDiff(nextmax, max));
}

// Calculate marginal state probs p(h) based on smoothed n-gram probabilities
void NGramShrink::PercolateStateProb(StateId st) {
  for (ArcIterator<StdExpandedFst> aiter(GetFst(), st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel == BackoffLabel()) continue;
    if (StateOrder(arc.nextstate) > StateOrder(st)) {
      shrink_state_[arc.nextstate].log_prob =
	shrink_state_[st].log_prob - arc.weight.Value();
      PercolateStateProb(arc.nextstate);
    }
  }
}

// Calculate marginal state probs and estimate total unigram count (odd w/KN)
void NGramShrink::CalculateStateProbs() {
  if (UnigramState() < 0) {
    // p(unigram state) = 1
    shrink_state_[GetFst().Start()].log_prob = 0;
  } else {
    // p(unigram state) = 1
    shrink_state_[UnigramState()].log_prob = 0;
    PercolateStateProb(UnigramState());
    // p(<s>) = p(</s>)
    shrink_state_[GetFst().Start()].log_prob =
      -GetFst().Final(UnigramState()).Value();
  }
  PercolateStateProb(GetFst().Start());
}

// Fill in relevant statistics for arc pruning at the state level
void NGramShrink::FillShrinkStateInfo() {
  for (StateId st = 0; st < ns_; ++st) {
    StateId bos = shrink_state_[st].backoff_state = GetBackoff(st, 0);
    if (bos < 0)  // No backoff from state -- i.e., unigram state
      continue;
    if (GetFst().Final(st) != StdArc::Weight::Zero())
      ++shrink_state_[bos].incoming_st_back_off;  // </s> backoff counter
    Matcher<StdFst> matcher(GetFst(), MATCH_INPUT);
    matcher.SetState(bos);
    for (ArcIterator<StdExpandedFst> aiter(GetFst(), st);
	 !aiter.Done();
	 aiter.Next()) {
      StdArc arc = aiter.Value();
      if (arc.ilabel == BackoffLabel()) continue;
      if (matcher.Find(arc.ilabel)) {  // increment backoff counter
	StdArc barc = matcher.Value();
	++BackedOffTo(bos, barc.ilabel, barc.nextstate);
      } else {
	LOG(FATAL) << "NGramShrink: No arc label match in backoff state";
      }
    }
  }
}

// Calculate and store statistics for scoring arc in pruning
int NGramShrink::AddArcStat(vector <ShrinkArcStats> *shrink_arcs,
			    StateId st, const StdArc *arc,
			    const StdArc *barc) {
  bool needed = false;
  StateId nextstate = -1;
  double hi_val, lo_val;
  if (arc) {
    // arc is needed even if score falls below threshold if:
    //   arc points to higher order (needed) state or is backed off to
    if ((StateOrder(st) < StateOrder(arc->nextstate) &&
	 !shrink_state_[arc->nextstate].state_dead) ||
	IsBackedOffTo(st, arc->ilabel, arc->nextstate)) {
      needed = true;
    }
    nextstate = barc->nextstate;
    hi_val = arc->weight.Value();  // higher order model value
    lo_val = barc->weight.Value();  // lower order model value
  } else {  // add pruned candidate for final cost at state (no nextstate)
    // final cost needed if backed off to (to avoid 'holes' in the model)
    if (shrink_state_[st].incoming_st_back_off > 0)
      needed = true;
    hi_val = GetFst().Final(st).Value();
    lo_val = GetFst().Final(shrink_state_[st].backoff_state).Value();
  }
  shrink_arcs->push_back(ShrinkArcStats(-hi_val, -lo_val,
					nextstate, needed));
  if (normalized_)
    SubtractFromBackoffNumDenom(hi_val, lo_val);
  return 1;
}

// Fill in relevant statistics for arc pruning for a particular state
size_t NGramShrink::FillShrinkArcInfo(vector <ShrinkArcStats> *shrink_arcs,
				   StateId st) {
  size_t candidates = 0;
  nlog_backoff_num_ = nlog_backoff_denom_ = 0;  // init backoff components
  Matcher<StdFst> matcher(GetFst(), MATCH_INPUT);  // to find backoff
  matcher.SetState(shrink_state_[st].backoff_state);
  for (ArcIterator<StdExpandedFst> aiter(GetFst(), st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if (arc.ilabel == BackoffLabel()) {
      shrink_arcs->push_back(ShrinkArcStats(0, 0, -1, true));  // placeholder
    } else if (matcher.Find(arc.ilabel)) {
      StdArc barc = matcher.Value();
      candidates += AddArcStat(shrink_arcs, st, &arc, &barc);
    } else {
      LOG(FATAL) << "NGramShrink: No arc label match in backoff state";
    }
  }
  // Final cost prune?
  if (GetFst().Final(st) != StdArc::Weight::Zero())
    candidates += AddArcStat(shrink_arcs, st, 0, 0);
  return candidates;
}

// Non-greedy comparison to threshold
size_t NGramShrink::ArcsToPrune(vector <ShrinkArcStats> *shrink_arcs,
				StateId st) const {
  size_t pruned_cnt = 0;
  double theta = GetTheta(st);
  if (theta == StdArc::Weight::Zero().Value()) return pruned_cnt;
  for (size_t i = 0; i < shrink_arcs->size(); ++i) {
    if (!(*shrink_arcs)[i].needed && !(*shrink_arcs)[i].pruned &&
	ShrinkScore(&(shrink_state_[st]), &((*shrink_arcs)[i])) < theta) {
      (*shrink_arcs)[i].pruned = true;
      ++pruned_cnt;
    }
  }
  return pruned_cnt;
}

// Evaluate arcs and select arcs to prune in greedy fashion
size_t NGramShrink::GreedyArcsToPrune(vector <ShrinkArcStats> *shrink_arcs,
				      StateId st) {
  ssize_t pruned_cnt = 0, last_prune_cnt = -1;
  while (last_prune_cnt < pruned_cnt) {  // while arcs continue to be pruned
    last_prune_cnt = pruned_cnt;
    double bestscore = GetTheta(st);  // score must be <= theta_ to be pruned
    ssize_t bestarc = -1;
    for (size_t i = 0; i < shrink_arcs->size(); ++i) {
      ShrinkArcStats arc = (*shrink_arcs)[i];
      if (!arc.needed && !arc.pruned) {
	double score = ShrinkScore(&(shrink_state_[st]), &arc);
	if (score <= bestscore) {  // tie goes to later arcs
	  bestscore = score;
	  bestarc = i;
	}
      }
    }
    if (bestarc >= 0) {  // found one to prune
      (*shrink_arcs)[bestarc].pruned = true;
      AddToBackoffNumDenom(-(*shrink_arcs)[bestarc].log_prob,
			  -(*shrink_arcs)[bestarc].log_backoff_prob);
      ++pruned_cnt;
    }
  }
  return pruned_cnt;
}

// For transitions selected to be pruned, point them to an unconnected state
size_t NGramShrink::PointPrunedArcs(vector <ShrinkArcStats> *shrink_arcs,
				 StateId st) {
  size_t acnt = 0, pruned_cnt = 0;
  for (MutableArcIterator<StdMutableFst>
	 aiter(GetMutableFst(), st);
       !aiter.Done();
       aiter.Next()) {
    StdArc arc = aiter.Value();
    if ((*shrink_arcs)[acnt].pruned) {
      arc.nextstate = dead_state_;  // point to unconnected state
      aiter.SetValue(arc);
      // decrement backoff counter
      --BackedOffTo(shrink_state_[st].backoff_state, arc.ilabel,
		    (*shrink_arcs)[acnt].backoff_dest);
      ++pruned_cnt;
    }
    ++acnt;
  }
  // if st is a final state and the final cost is marked to be pruned, prune
  if (acnt < shrink_arcs->size() && (*shrink_arcs)[acnt].pruned) {
    --shrink_state_[GetBackoff(st, 0)].incoming_st_back_off;
    GetMutableFst()->SetFinal(st, StdArc::Weight::Zero());
    ++pruned_cnt;
  }
  return pruned_cnt;
}

// Evaluate transitions from state and prune in greedy fashion
void NGramShrink::PruneState(StateId st) {
  vector<ShrinkArcStats> shrink_arcs;
  size_t candidate_prune = FillShrinkArcInfo(&shrink_arcs, st);
  size_t pruned_cnt = ChooseArcsToPrune(&shrink_arcs, st);
  if (pruned_cnt > 0) {
    size_t xyz = PointPrunedArcs(&shrink_arcs, st);
    if (xyz != pruned_cnt) {
      cerr << pruned_cnt << " " << xyz << endl;
      LOG(FATAL) << "NGramShrink: Pruned count mismatch.";
    }
    if (pruned_cnt == candidate_prune)  // all candidate arcs pruned
      shrink_state_[st].state_dead = true;  // state becomes a dead state
  }
}

// Find unpruned arcs pointing to unconnected states and point them elsewhere
void NGramShrink::PointArcsAwayFromDead() {
  for (StateId st = 0; st < ns_; ++st) {
    if (shrink_state_[st].state_dead)
      continue;
    for (MutableArcIterator<StdMutableFst>
	   aiter(GetMutableFst(), st);
	 !aiter.Done();
	 aiter.Next()) {
      StdArc arc = aiter.Value();
      if (arc.nextstate != dead_state_) {
	while (shrink_state_[arc.nextstate].state_dead) {
	  arc.nextstate = GetBackoff(arc.nextstate, 0);
	  aiter.SetValue(arc);
	}
      }
    }
  }
  PointDeadBackoffArcs();
}

// Map backoff arcs of dead states to dead_state_ (except for start state)
void NGramShrink::PointDeadBackoffArcs() {
  for (StateId st = 0; st < ns_; ++st) {
    if (!shrink_state_[st].state_dead || st == GetFst().Start())
      continue;
    MutableArcIterator<StdMutableFst>
      aiter(GetMutableFst(), st);
    if (FindMutableArc(&aiter, BackoffLabel())) {
      StdArc arc = aiter.Value();
      arc.nextstate = dead_state_;
      aiter.SetValue(arc);
    }
    else {
      LOG(FATAL) << "NGramShrink: No backoff arc in dead state";
    }
  }
}

}  // namespace ngram
