// ngram-shrink.h
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
// NGram model class for shrinking or pruning the model

#ifndef NGRAM_NGRAM_SHRINK_H__
#define NGRAM_NGRAM_SHRINK_H__

#include <unordered_map>

#include <ngram/ngram-mutable-model.h>

namespace ngram {

class NGramShrink : public NGramMutableModel {
 public:
  typedef StdArc::StateId StateId;
  typedef StdArc::Label Label;
  typedef StdArc::Weight Weight;

  // Construct an NGramShrink object, including an NGramModel and parameters
  NGramShrink(StdMutableFst *infst, int shrink_opt = 0,
	      double tot_uni = -1.0, Label backoff_label = 0,
	      double norm_eps = kNormEps, bool check_consistency = false);

  // Shrink n-gram model, based on initialized parameters
  void ShrinkNGramModel(bool require_norm);

  virtual ~NGramShrink() { }

 protected:
  // Data representation for an arc being considered for pruning
  struct ShrinkArcStats {
    double log_prob;  // log probability of word given history
    double log_backoff_prob;  // log probability of word given backoff history
    Label label;  // arc label
    StateId backoff_dest;  // destination state of backoff arc
    bool needed;  // is the current arc needed within the automaton?
    bool pruned;  // has the current arc been pruned already by shrinking?

    ShrinkArcStats(double lp, double lbp, Label lab, StateId dest,
		   bool needed)
        : log_prob(lp), log_backoff_prob(lbp), label(lab),
      backoff_dest(dest), needed(needed), pruned(false) {}
  };

  // Data representation for a state with arcs being considered for pruning
  struct ShrinkStateStats {
    double log_prob;  // log probability of history represented by state
    StateId state;          // state ID of current state
    StateId backoff_state;  // state ID of backoff state
    bool state_dead;  // store whether state is to be removed from model
    // # of arcs that back off thru incoming arc. This is only for incoming
    // arcs that increase in state order and thus are uniquely determined
    // by their destination state.
    // NB: dest. state uniquely determines arc label in this case.
    size_t incoming_backed_off;
    // # of final states that backoff to state
    size_t incoming_st_back_off;

    ShrinkStateStats()
        : log_prob(0), state(kNoStateId), backoff_state(kNoStateId),
          state_dead(false), incoming_backed_off(0), incoming_st_back_off(0) {}
  };

  // Provides the score provided to arc for particular shrinking method
  // Need to override in derived class for anything but count pruning
  // Default calculates count for normalized model; raw count for unnormalized
  virtual double ShrinkScore(const ShrinkStateStats &state,
			     const ShrinkArcStats &arc) const {
    if (!normalized_) return arc.log_prob;  // unnormalized log count
    return arc.log_prob + state.log_prob + log(total_unigram_count_);
  }

  // Provides the threshold for comparing to the scores to decide to prune
  // Required from derived classes
  virtual double GetTheta(StateId state) const = 0;

  // Calculates the new backoff weight if arc removed
  double CalcNewLogBackoff(const ShrinkArcStats &arc) const {
    return NegLogSum(nlog_backoff_denom_, -arc.log_backoff_prob) -
      NegLogSum(nlog_backoff_num_, -arc.log_prob);
  }

  // Provides access to total unigram count
  double GetTotalUnigramCount() const {
    return total_unigram_count_;
  }

  // Provides access to negative log numerator of the backoff
  double GetNLogBackoffNum() const {
    return nlog_backoff_num_;
  }

  // Provides access to negative log denominator of the backoff
  double GetNLogBackoffDenom() const {
    return nlog_backoff_denom_;
  }

  private:
  void FillStateProbs();

  struct BackedOffToHash {
    size_t operator()(const pair<StateId, Label> &p) const {
      return p.first + p.second * 7853;
    }
  };

  // transition from 'st' to 'dest' labeled with 'label'.
  size_t &BackedOffTo(StateId st, Label label, StateId dest) {
    if (StateOrder(st) < StateOrder(dest))  // arc unique to dest; store there
      return shrink_state_[dest].incoming_backed_off;
    else   // o.w. hash it
      return backed_off_to_[make_pair(st, label)];  // inserts if needed.
  }

  // Efficiently checks if non-zero BackedOffTo() (no side-effects)
  bool IsBackedOffTo(StateId st, Label label, StateId dest) const {
    if (StateOrder(st) < StateOrder(dest))
      return shrink_state_[dest].incoming_backed_off > 0;
    else {
      typedef unordered_map< pair<StateId, Label>, size_t, BackedOffToHash> B;
      B::const_iterator it = backed_off_to_.find(make_pair(st, label));
      if (it == backed_off_to_.end())
        return false;
      else
        return it->second > 0;
    }
  }

  // Fill in relevant statistics for arc pruning at the state level
  void FillShrinkStateInfo();

  // Adds probs to backoff numerator and denominator
  void AddToBackoffNumDenom(double num_upd_val, double denom_upd_val) {
    nlog_backoff_num_ = NegLogSum(nlog_backoff_num_, num_upd_val);
    nlog_backoff_denom_ = NegLogSum(nlog_backoff_denom_, denom_upd_val);
  }

  // Subtracts probs from backoff numerator and denominator
  void UpdateBackoffNumDenom(double num_upd_val,
			     double denom_upd_val,
			     double *neg_log_correct_num,
			     double *neg_log_correct_denom) {
    nlog_backoff_num_ = NegLogSum(nlog_backoff_num_, num_upd_val,
				  neg_log_correct_num);
    nlog_backoff_denom_ = NegLogSum(nlog_backoff_denom_, denom_upd_val,
				    neg_log_correct_denom);
  }

  // Calculate and store statistics for scoring arc in pruning
  int AddArcStat(vector <ShrinkArcStats> *shrink_arcs, StateId st,
		 const StdArc *arc, const StdArc *barc) ;

  // Fill in relevant statistics for arc pruning for a particular state
  size_t FillShrinkArcInfo(vector <ShrinkArcStats> *shrink_arcs, StateId st);

  // Non-greedy comparison to threshold, such as used for count pruning
  size_t ArcsToPrune(vector <ShrinkArcStats> *shrink_arcs, StateId st) const;

  // Evaluate arcs and select arcs to prune in greedy fashion
  size_t GreedyArcsToPrune(vector <ShrinkArcStats> *shrink_arcs, StateId st);

  // Evaluate arcs and select arcs to prune
  size_t ChooseArcsToPrune(vector <ShrinkArcStats> *shrink_arcs, StateId st) {
    if (shrink_opt_ < 2)
      return ArcsToPrune(shrink_arcs, st);
    else
      return GreedyArcsToPrune(shrink_arcs, st);
  }

  // For transitions selected to be pruned, point them to an unconnected state
  size_t PointPrunedArcs(const vector <ShrinkArcStats> &shrink_arcs,
                         StateId st);

  // Evaluate transitions from state and prune in greedy fashion
  void PruneState(StateId st);

  // Evaluate states from highest order to lowest order for shrinking.
  void PruneModel() {
    for (int order = HiOrder(); order > 1; --order) {
      for (StateId st = 0; st < ns_; ++st) {
	if (StateOrder(st) == order)  // current order
	  PruneState(st);
      }
    }
  }

  // Find unpruned arcs pointing to unconnected states and point them elsewhere
  void PointArcsAwayFromDead();

  // Map backoff arcs of dead states to dead_state_ (except for start state)
  void PointDeadBackoffArcs();

  bool normalized_;  // Whether the NGram model is initially normalized
  int shrink_opt_;   // Opt. level: Range 0 (fastest) to 2 (most accurate)
  double total_unigram_count_;  // Total unigram counts
  double nlog_backoff_num_;  // numerator of backoff weight
  double nlog_backoff_denom_;  // denominator of backoff weight
  StateId ns_;  // Original number of states in the model
  StateId dead_state_;  // Sink state dest. for pruned arcs (not connected)
  vector<ShrinkStateStats> shrink_state_;
  unordered_map<pair<StateId, Label>, size_t, BackedOffToHash> backed_off_to_;

  DISALLOW_COPY_AND_ASSIGN(NGramShrink);
};

}  // namespace ngram

#endif  // NGRAM_NGRAM_SHRINK_H__
