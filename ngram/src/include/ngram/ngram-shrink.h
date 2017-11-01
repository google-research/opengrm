
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
// Copyright 2005-2016 Brian Roark and Google, Inc.
// NGram model class for shrinking or pruning the model.

#ifndef NGRAM_NGRAM_SHRINK_H_
#define NGRAM_NGRAM_SHRINK_H_

#include <sstream>

#include <ngram/ngram-mutable-model.h>
#include <ngram/util.h>
#include <unordered_map>

namespace ngram {

using std::stringstream;

template <class Arc>
class NGramShrink : public NGramMutableModel<Arc> {
 public:
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;

  using NGramModel<Arc>::Error;
  using NGramMutableModel<Arc>::HiOrder;
  using NGramMutableModel<Arc>::CheckNormalization;
  using NGramMutableModel<Arc>::GetMutableFst;
  using NGramMutableModel<Arc>::EstimateTotalUnigramCount;
  using NGramMutableModel<Arc>::InitModel;
  using NGramMutableModel<Arc>::RecalcBackoff;
  using NGramMutableModel<Arc>::CalculateStateProbs;
  using NGramMutableModel<Arc>::GetFst;
  using NGramMutableModel<Arc>::GetExpandedFst;
  using NGramMutableModel<Arc>::BackoffLabel;
  using NGramMutableModel<Arc>::UnigramState;
  using NGramMutableModel<Arc>::StateOrder;
  using NGramMutableModel<Arc>::GetBackoff;
  using NGramMutableModel<Arc>::CalcBONegLogSums;
  using NGramMutableModel<Arc>::CalculateBackoffFactors;
  using NGramMutableModel<Arc>::ScalarValue;
  using NGramMutableModel<Arc>::FindMutableArc;

  // Constructs an NGramShrink object, including an NGramModel and parameters.
  explicit NGramShrink(MutableFst<Arc> *infst, int shrink_opt = 0,
                       double tot_uni = -1.0, Label backoff_label = 0,
                       double norm_eps = kNormEps,
                       bool check_consistency = false, bool norm = true);

  // Shrinks n-gram model, based on initialized parameters.  No ngrams smaller
  // than min_order will be pruned; min_order must be at least 2 (the default
  // value).
  bool ShrinkNGramModel(bool require_norm, int min_order = 2);

  // Calculates shrinking scores for all ngrams, without actually pruning (yet).
  void CalculateShrinkScores(bool require_norm);

  // Provides label vectors and/or vector of their shrink scores.
  void GetNGramsAndOrScores(std::vector<std::vector<Label>> *ngrams,
                            std::vector<double> *scores, bool collect_unigrams);

  // Provides label vectors and/or vector of their shrink scores.
  void GetNGramsAndOrScoresMinOrder(std::vector<std::vector<Label>> *ngrams,
                                    std::vector<double> *scores, int min_order);

  virtual ~NGramShrink() {}

 protected:
  // Data representation for an arc being considered for pruning.
  struct ShrinkArcStats {
    double log_prob;          // Log probability of word given history.
    double log_backoff_prob;  // Log probability of word given backoff history.
    double shrink_score;      // Calculated score for shrinking.
    Label label;              // Arc label.
    StateId backoff_dest;     // Destination state of backoff arc.
    bool needed;              // Is the current arc needed within the automaton?
    bool pruned;  // Has the current arc been pruned already by shrinking?

    ShrinkArcStats(double lp, double lbp, Label lab, StateId dest, bool needed)
        : log_prob(lp),
          log_backoff_prob(lbp),
          shrink_score(0.0),
          label(lab),
          backoff_dest(dest),
          needed(needed),
          pruned(false) {}
  };

  // Data representation for a state with arcs being considered for pruning.
  struct ShrinkStateStats {
    double log_prob;        // Log probability of history represented by state.
    StateId state;          // State ID of current state.
    StateId backoff_state;  // State ID of backoff state.
    StateId prefix_state;   // State ID of prior state on ascending path.
    Label incoming_label;   // Label of arc leading to state on ascending path.
    bool state_dead;        // Store whether state is to be removed from model.
    // # of arcs that back off thru incoming arc. This is only for incoming
    // arcs that increase in state order and thus are uniquely determined
    // by their destination state.
    // NB: destination state uniquely determines arc label in this case.
    size_t incoming_backed_off;
    // # of final states that backoff to state.
    size_t incoming_st_back_off;

    ShrinkStateStats()
        : log_prob(0),
          state(kNoStateId),
          backoff_state(kNoStateId),
          prefix_state(kNoStateId),
          incoming_label(kNoLabel),
          state_dead(false),
          incoming_backed_off(0),
          incoming_st_back_off(0) {}
  };

  // Provides the score provided to arc for particular shrinking method. One
  // must override this in any derived class for anything but count pruning.
  // Default calculates count for normalized model; raw count for unnormalized.
  virtual double ShrinkScore(const ShrinkStateStats &state,
                             const ShrinkArcStats &arc) const {
    if (!normalized_) {
      return arc.log_prob;  // unnormalized log count
    } else if (arc.log_prob == -ScalarValue(Arc::Weight::Zero()) ||
               state.log_prob == -ScalarValue(Arc::Weight::Zero()) ||
               total_unigram_count_ <= 0.0) {
      return -ScalarValue(Arc::Weight::Zero());
    }
    return arc.log_prob + state.log_prob + log(total_unigram_count_);
  }

  // Provides the threshold for comparing to the scores to decide to prune.
  // Required from derived classes.
  virtual double GetTheta(StateId state) const = 0;

  // Returns the theta value that guarantees at most target_number_of_ngrams,
  // while optionally specifying a minimum pruning order.
  double ThetaForMaxNGrams(int target_number_of_ngrams, int min_order = 2);

  // Calculates the new backoff weight if arc removed.
  double CalcNewLogBackoff(const ShrinkArcStats &arc) const {
    return NegLogSum(nlog_backoff_denom_, -arc.log_backoff_prob) -
           NegLogSum(nlog_backoff_num_, -arc.log_prob);
  }

  // Provides access to total unigram count.
  double GetTotalUnigramCount() const { return total_unigram_count_; }

  // Provides access to negative log numerator of the backoff.
  double GetNLogBackoffNum() const { return nlog_backoff_num_; }

  // Provides access to negative log denominator of the backoff.
  double GetNLogBackoffDenom() const { return nlog_backoff_denom_; }

 private:
  void FillStateProbs();

  struct StateLabelHash {
    size_t operator()(const std::pair<StateId, Label> &p) const {
      return p.first + p.second * 7853;
    }
  };

  // Fills n-gram label vector in correct order via recursive function.
  void AddStateNGramLabels(StateId st, std::vector<Label> *ngram_labels);

  // Finds an entry in the map with shrink score or produces fatal error.
  double FindOrDieShrinkScore(StateId st, Label label);

  // Transition from 'st' to 'dest' labeled with 'label'.
  size_t &BackedOffTo(StateId st, Label label, StateId dest) {
    if (StateOrder(st) < StateOrder(dest)) {           // Arc unique
      return shrink_state_[dest].incoming_backed_off;  // to dest., store there.
    } else {                                             // o.w. hash it.
      return backed_off_to_[std::make_pair(st, label)];  // Inserts if needed.
    }
  }

  // Efficiently checks if non-zero BackedOffTo() (no side-effects).
  bool IsBackedOffTo(StateId st, Label label, StateId dest) const {
    if (StateOrder(st) < StateOrder(dest))
      return shrink_state_[dest].incoming_backed_off > 0;
    else {
      auto it = backed_off_to_.find(std::make_pair(st, label));
      if (it == backed_off_to_.end())
        return false;
      else
        return it->second > 0;
    }
  }

  // Fills in relevant statistics for arc pruning at the state level.
  void FillShrinkStateInfo();

  // Adds probabilities to backoff numerator and denominator.
  void AddToBackoffNumDenom(double num_upd_val, double denom_upd_val) {
    nlog_backoff_num_ = NegLogSum(nlog_backoff_num_, num_upd_val);
    nlog_backoff_denom_ = NegLogSum(nlog_backoff_denom_, denom_upd_val);
  }

  // Subtracts probabilities from backoff numerator and denominator.
  void UpdateBackoffNumDenom(double num_upd_val, double denom_upd_val,
                             double *neg_log_correct_num,
                             double *neg_log_correct_denom) {
    nlog_backoff_num_ =
        NegLogSum(nlog_backoff_num_, num_upd_val, neg_log_correct_num);
    nlog_backoff_denom_ =
        NegLogSum(nlog_backoff_denom_, denom_upd_val, neg_log_correct_denom);
  }

  // Updates maximum score for a given label leaving a given state, and returns
  // the maximum.
  double UpdateScoreHash(StateId st, Label label, double shrink_score);

  // Retrieves shrink score, calculating if requested.
  double GetShrinkScore(const ShrinkArcStats &arc, StateId st, Label label,
                        bool calc_score);

  // Calculates and store statistics for scoring arc in pruning.
  int AddArcStat(std::vector<ShrinkArcStats> *shrink_arcs, StateId st,
                 const Arc *arc, const Arc *barc, bool calc_score);

  // Fills in relevant statistics for arc pruning for a particular state.
  size_t FillShrinkArcInfo(std::vector<ShrinkArcStats> *shrink_arcs, StateId st,
                           bool calc_score);

  // Calculates scores of all arcs leaving all states in model.
  void ScoreAllArcs();

  // Non-greedy comparison to threshold, such as used for count pruning.
  size_t ArcsToPrune(std::vector<ShrinkArcStats> *shrink_arcs,
                     StateId st) const;

  // Evaluates arcs and select arcs to prune in greedy fashion.
  size_t GreedyArcsToPrune(std::vector<ShrinkArcStats> *shrink_arcs,
                           StateId st);

  // Evaluates arcs and select arcs to prune.
  size_t ChooseArcsToPrune(std::vector<ShrinkArcStats> *shrink_arcs,
                           StateId st) {
    if (shrink_opt_ < 2)
      return ArcsToPrune(shrink_arcs, st);
    else
      return GreedyArcsToPrune(shrink_arcs, st);
  }

  // For transitions selected to be pruned, point them to an unconnected state
  size_t PointPrunedArcs(const std::vector<ShrinkArcStats> &shrink_arcs,
                         StateId st);

  // Evaluate transitions from state and prune in greedy fashion
  void PruneState(StateId st);

  // Evaluate states from highest order to lowest order for shrinking.
  void PruneModel(int requested_min_order) {
    int min_order = requested_min_order;
    if (requested_min_order < 2) {
      LOG(WARNING) << "Minimum order for pruning below 2.  Bigrams are the "
                      "minimum possible, so resetting this parameter to 2.";
      min_order = 2;
    }
    for (int order = HiOrder(); order >= min_order; --order) {
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
  bool norm_;        // Whether to normalize the result (if input normalized)
  int shrink_opt_;   // Opt. level: Range 0 (fastest) to 2 (most accurate)
  double total_unigram_count_;  // Total unigram counts
  double nlog_backoff_num_;     // numerator of backoff weight
  double nlog_backoff_denom_;   // denominator of backoff weight
  StateId ns_;                  // Original number of states in the model
  StateId dead_state_;  // Sink state dest. for pruned arcs (not connected)
  std::vector<ShrinkStateStats> shrink_state_;
  std::unordered_map<std::pair<StateId, Label>, double, StateLabelHash>
      max_shrink_score_;
  std::unordered_map<std::pair<StateId, Label>, size_t, StateLabelHash>
      backed_off_to_;
};

// Construct an NGramShrink object, including an NGramMutableModel
// and parameters.
template <class Arc>
NGramShrink<Arc>::NGramShrink(MutableFst<Arc> *infst, int shrink_opt,
                              double tot_uni, Label backoff_label,
                              double norm_eps, bool check_consistency,
                              bool norm)
    : NGramMutableModel<Arc>(infst, backoff_label, norm_eps, check_consistency),
      normalized_(CheckNormalization()),
      norm_(norm),
      shrink_opt_(shrink_opt),
      total_unigram_count_(tot_uni),
      ns_(infst->NumStates()),
      dead_state_(GetMutableFst()->AddState()) {
  // set switch if inf backoff costs
  NGramMutableModel<Arc>::SetAllowInfiniteBO();
  for (StateId st = 0; st < ns_; ++st)
    shrink_state_.push_back(ShrinkStateStats());
}

// Calculates scores of all arcs leaving all states in model.
template <class Arc>
void NGramShrink<Arc>::ScoreAllArcs() {
  for (int order = HiOrder(); order > 1; --order) {
    for (StateId st = 0; st < ns_; ++st) {
      if (StateOrder(st) == order) {  // current order
        std::vector<ShrinkArcStats> shrink_arcs;
        FillShrinkArcInfo(&shrink_arcs, st, true);
        if (Error()) return;
      }
    }
  }
}

// Calculates shrink scores for all ngrams in a model, stores in a hash.
template <class Arc>
void NGramShrink<Arc>::CalculateShrinkScores(bool require_norm) {
  if (max_shrink_score_.size() > 0) return;  // Scores already calculated.
  if (normalized_) {                // only required for normalized models
    FillStateProbs();               // calculate p(h)
    if (total_unigram_count_ <= 0)  // auto derive unigram count if req'd
      total_unigram_count_ = EstimateTotalUnigramCount();
  } else if (require_norm) {
    NGRAMERROR() << "NGramShrink: Model not normalized;"
                 << " Model must be normalized for this shrinking method";
    NGramModel<Arc>::SetError();
    return;
  }
  FillShrinkStateInfo();  // collects state information
  if (Error()) {
    NGRAMERROR() << "NGramShrink: Error in collecting state information";
    return;
  }
  ScoreAllArcs();
}

// Shrink n-gram model, based on initialized parameters
template <class Arc>
bool NGramShrink<Arc>::ShrinkNGramModel(bool require_norm, int min_order) {
  CalculateShrinkScores(require_norm);  // Calculates scores for all ngrams.
  if (Error()) {
    NGRAMERROR() << "NGramShrink: Error in calculating shrink scores";
    return false;
  }
  PruneModel(min_order);  // prunes arcs and points to unconnected state
  if (Error()) {
    NGRAMERROR() << "NGramShrink: Error in pruning model";
    return false;
  }
  PointArcsAwayFromDead();      // points unpruned arcs to connected states
  if (Error()) {
    NGRAMERROR() << "NGramShrink: Error in redirecting arcs";
    return false;
  }
  Connect(GetMutableFst());     // removes pruned arcs and dead states
  InitModel();                  // re-calcs state info
  if (Error()) {
    NGRAMERROR() << "NGramShrink: Error in recalculating state info";
    return false;
  } else if (normalized_ && norm_) {  // only needed for normalized models
    RecalcBackoff();                  // re-calcs backoff weights
    if (!CheckNormalization()) {      // model should be normalized
      NGRAMERROR() << "NGramShrink: Pruned model not fully normalized";
      return false;
    }
  }
  return true;
}

template <class Arc>
void NGramShrink<Arc>::FillStateProbs() {
  std::vector<double> probs;
  CalculateStateProbs(&probs);
  for (StateId st = 0; st < ns_; ++st)
    shrink_state_[st].log_prob = log(probs[st]);
}

// Fill in relevant statistics for arc pruning at the state level
template <class Arc>
void NGramShrink<Arc>::FillShrinkStateInfo() {
  for (StateId st = 0; st < ns_; ++st) {
    shrink_state_[st].state = st;
    StateId bos = shrink_state_[st].backoff_state = GetBackoff(st, 0);
    Matcher<Fst<Arc>> matcher(GetFst(), MATCH_INPUT);
    if (bos >= 0) {
      if (GetFst().Final(st) != Arc::Weight::Zero())
        ++shrink_state_[bos].incoming_st_back_off;  // </s> backoff counter
      matcher.SetState(bos);
      shrink_state_[st].state_dead = GetFst().Final(st) == Arc::Weight::Zero();
    }
    for (ArcIterator<ExpandedFst<Arc>> aiter(GetExpandedFst(), st);
         !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
      if (arc.ilabel == BackoffLabel()) continue;

      // if ascending, record prefix state and incoming label.
      if (StateOrder(st) < StateOrder(arc.nextstate)) {
        shrink_state_[arc.nextstate].prefix_state = st;
        shrink_state_[arc.nextstate].incoming_label = arc.ilabel;
      }
      if (bos < 0) continue;  // that is all the work at the unigram state.
      shrink_state_[st].state_dead = false;
      if (matcher.Find(arc.ilabel)) {  // increment backoff counter
        Arc barc = matcher.Value();
        ++BackedOffTo(bos, barc.ilabel, barc.nextstate);
      } else {
        NGRAMERROR() << "NGramShrink: No arc label match in backoff state";
        NGramModel<Arc>::SetError();
        return;
      }
    }
  }
}

// Fills ngram label vector in correct order via recursive function.
template <class Arc>
void NGramShrink<Arc>::AddStateNGramLabels(StateId st,
                                           vector<Label> *ngram_labels) {
  if (shrink_state_[st].prefix_state != kNoStateId) {
    AddStateNGramLabels(shrink_state_[st].prefix_state, ngram_labels);
    ngram_labels->push_back(shrink_state_[st].incoming_label);
  } else if (st == GetFst().Start() && UnigramState() >= 0) {
    ngram_labels->push_back(0);
  }
}

// Finds an entry in the hash table with shrink score or fatal error.
template <class Arc>
double NGramShrink<Arc>::FindOrDieShrinkScore(StateId st, Label label) {
  auto map_iterator = max_shrink_score_.find(std::make_pair(st, label));
  if (map_iterator == max_shrink_score_.end()) {
    NGRAMERROR() << "NGramShrink: score has not been calculated yet.";
    NGramModel<Arc>::SetError();
    return 0.0;
  }
  return map_iterator->second;
}

// Provides ngram label vectors and/or vector of their shrink scores.
template <class Arc>
void NGramShrink<Arc>::GetNGramsAndOrScores(vector<vector<Label>> *ngrams,
                                            std::vector<double> *scores,
                                            bool collect_unigrams) {
  // Assigns min_order based on collect_unigrams, to preserve behavior.
  GetNGramsAndOrScoresMinOrder(ngrams, scores,
                               /* min_order = */ collect_unigrams ? 1 : 2);
}

// Provides ngram label vectors and/or vector of their shrink scores.
template <class Arc>
void NGramShrink<Arc>::GetNGramsAndOrScoresMinOrder(
    vector<vector<Label>> *ngrams, std::vector<double> *scores, int min_order) {
  if (ngrams == nullptr && scores == nullptr) return;
  for (StateId st = 0; st < ns_; ++st) {
    std::vector<Label> state_ngram;
    AddStateNGramLabels(st, &state_ngram);  // Labels of words leading to state.

    // Skips unigrams if min_order higher, matches prior behavior.
    if (state_ngram.empty() && min_order > 1) continue;
    std::vector<Label> to_update;
    for (ArcIterator<ExpandedFst<Arc>> aiter(GetExpandedFst(), st);
         !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
      if (arc.ilabel == BackoffLabel()) continue;
      to_update.push_back(arc.ilabel);
    }
    // End-of-string ngram
    if (ScalarValue(GetFst().Final(st)) != ScalarValue(Arc::Weight::Zero()))
      to_update.push_back(kNoLabel);
    for (size_t idx = 0; idx < to_update.size(); ++idx) {
      if (ngrams != nullptr) {
        std::vector<Label> ngram_labels = state_ngram;
        ngram_labels.push_back(to_update[idx]);
        ngrams->push_back(ngram_labels);
      }
      if (scores != nullptr) {
        if (state_ngram.empty()) {
          // Unigram score is 0.0 if included, matching prior behavior.
          scores->push_back(0.0);
        } else if (state_ngram.size() < min_order - 1) {
          // Excludes min_order n-grams by assigning max possible shrink score.
          scores->push_back(std::numeric_limits<double>::max());
        } else {
          scores->push_back(FindOrDieShrinkScore(st, to_update[idx]));
          if (Error()) return;
        }
      }
    }
  }
}

// Updates maximum score for a given label leaving a given state. Returns max.
template <class Arc>
double NGramShrink<Arc>::UpdateScoreHash(StateId st, Label label,
                                         double shrink_score) {
  double &max_shrink_score = max_shrink_score_.emplace(  // Insert if not found.
      std::make_pair(st, label), shrink_score).first->second;
  if (shrink_score > max_shrink_score) {
    max_shrink_score = shrink_score;
  }
  return max_shrink_score;
}

// Retrieves shrink score, calculating if requested.  If calculating the score,
// also updates suffix and prefix ngram maximum score.
template <class Arc>
double NGramShrink<Arc>::GetShrinkScore(const ShrinkArcStats &arc, StateId st,
                                        Label label, bool calc_score) {
  double shrink_score = 0.0;
  if (calc_score) {  // Calculates local score and compares with maximum.
    shrink_score =
        UpdateScoreHash(st, label, ShrinkScore(shrink_state_[st], arc));

    // Updates suffix ngram with maximum.
    UpdateScoreHash(shrink_state_[st].backoff_state, label, shrink_score);

    // Updates prefix ngram with maximum.
    if (shrink_state_[st].prefix_state != kNoStateId)
      UpdateScoreHash(shrink_state_[st].prefix_state,
                      shrink_state_[st].incoming_label, shrink_score);
  } else {
    shrink_score = FindOrDieShrinkScore(st, label);
  }
  return shrink_score;
}

// Calculate and store statistics for scoring arc in pruning
template <class Arc>
int NGramShrink<Arc>::AddArcStat(vector<ShrinkArcStats> *shrink_arcs,
                                 StateId st, const Arc *arc, const Arc *barc,
                                 bool calc_score) {
  bool needed = false;
  StateId nextstate = kNoStateId;
  double hi_val, lo_val;
  Label label = kNoLabel;

  if (arc) {
    // arc is needed even if score falls below threshold if:
    //   arc points to higher order (needed) state or is backed off to
    if ((StateOrder(st) < StateOrder(arc->nextstate) &&
         !shrink_state_[arc->nextstate].state_dead) ||
        IsBackedOffTo(st, arc->ilabel, arc->nextstate)) {
      needed = true;
    }
    nextstate = barc->nextstate;
    hi_val = ScalarValue(arc->weight);   // higher order model value
    lo_val = ScalarValue(barc->weight);  // lower order model value
    label = arc->ilabel;
  } else {  // add pruned candidate for final cost at state (no nextstate)
    // final cost needed if backed off to (to avoid 'holes' in the model)
    if (shrink_state_[st].incoming_st_back_off > 0) needed = true;
    hi_val = ScalarValue(GetFst().Final(st));
    lo_val = ScalarValue(GetFst().Final(shrink_state_[st].backoff_state));
  }
  int arc_index = shrink_arcs->size();
  shrink_arcs->push_back(
      ShrinkArcStats(-hi_val, -lo_val, label, nextstate, needed));
  (*shrink_arcs)[arc_index].shrink_score =
      GetShrinkScore((*shrink_arcs)[arc_index], st, label, calc_score);
  return 1;
}

// Fill in relevant statistics for arc pruning for a particular state
template <class Arc>
size_t NGramShrink<Arc>::FillShrinkArcInfo(vector<ShrinkArcStats> *shrink_arcs,
                                           StateId st, bool calc_score) {
  size_t candidates = 0;
  if (normalized_) {
    double hi_neglog_sum, low_neglog_sum;
    CalcBONegLogSums(st, &hi_neglog_sum, &low_neglog_sum);
    if (Error()) return candidates;
    CalculateBackoffFactors(hi_neglog_sum, low_neglog_sum, &nlog_backoff_num_,
                            &nlog_backoff_denom_);
  }
  Matcher<Fst<Arc>> matcher(GetFst(), MATCH_INPUT);  // to find backoff
  matcher.SetState(shrink_state_[st].backoff_state);
  for (ArcIterator<ExpandedFst<Arc>> aiter(GetExpandedFst(), st); !aiter.Done();
       aiter.Next()) {
    Arc arc = aiter.Value();
    if (arc.ilabel == BackoffLabel()) {
      // placeholder
      shrink_arcs->push_back(
          ShrinkArcStats(0, 0, arc.ilabel, kNoStateId, true));
    } else if (matcher.Find(arc.ilabel)) {
      Arc barc = matcher.Value();
      candidates += AddArcStat(shrink_arcs, st, &arc, &barc, calc_score);
    } else {
      NGRAMERROR() << "NGramShrink: No arc label match in backoff state";
      NGramModel<Arc>::SetError();
      return candidates;
    }
  }
  // Final cost prune?
  if (ScalarValue(GetFst().Final(st)) != ScalarValue(Arc::Weight::Zero()))
    candidates += AddArcStat(shrink_arcs, st, 0, 0, calc_score);
  return candidates;
}

// Returns the theta value that guarantees at most target_number_of_ngrams,
// taking into account the minimum order to prune.
template <class Arc>
double NGramShrink<Arc>::ThetaForMaxNGrams(int target_number_of_ngrams,
                                           int min_order) {
  NGramShrink<Arc>::CalculateShrinkScores(true);
  if (Error()) {
    NGRAMERROR() << "ThetaForMaxNGrams: Error in calculating shrink scores";
    return 0.0;
  }
  std::vector<double> scores;  // Only care about scores, not ngram identities.
  NGramShrink<Arc>::GetNGramsAndOrScores(nullptr, &scores, min_order);
  if (Error()) {
    NGRAMERROR() << "ThetaForMaxNGrams: Error in getting ngram scores";
    return 0.0;
  }
  if (scores.size() == 0 || UnigramState() < 0)  // No ngrams to prune.
    return 0.0;
  std::sort(scores.begin(), scores.end());

  // Unigram count is number of arcs leaving unigram + final cost.
  target_number_of_ngrams -= GetFst().NumArcs(UnigramState()) + 1;
  if (target_number_of_ngrams < 0) target_number_of_ngrams = 0;

  // Set threshold index to largest score to be pruned.
  int threshold_index = scores.size() - target_number_of_ngrams - 1;
  if (threshold_index < 0)
    return scores[0] - 1.0;  // Sets threshold less than the lowest value.
  double theta = scores[threshold_index];
  while (threshold_index < scores.size() && scores[threshold_index] == theta) {
    threshold_index++;
  }
  if (threshold_index >= scores.size()) {  // Sets theta more than max.
    ++theta;
  } else {  // Sets theta midway between last to keep and first to prune.
    theta += scores[threshold_index];
    theta /= 2;
  }
  return theta;
}

// Non-greedy comparison to threshold
template <class Arc>
size_t NGramShrink<Arc>::ArcsToPrune(vector<ShrinkArcStats> *shrink_arcs,
                                     StateId st) const {
  size_t pruned_cnt = 0;
  double theta = GetTheta(st);
  if (theta == ScalarValue(Arc::Weight::Zero())) return pruned_cnt;
  for (size_t i = 0; i < shrink_arcs->size(); ++i) {
    if (!(*shrink_arcs)[i].pruned && !(*shrink_arcs)[i].needed &&
        (*shrink_arcs)[i].shrink_score < theta) {
      (*shrink_arcs)[i].pruned = true;
      ++pruned_cnt;
    }
  }
  return pruned_cnt;
}

// Evaluate arcs and select arcs to prune in greedy fashion
template <class Arc>
size_t NGramShrink<Arc>::GreedyArcsToPrune(vector<ShrinkArcStats> *shrink_arcs,
                                           StateId st) {
  ssize_t pruned_cnt = 0, last_prune_cnt = -1;
  while (last_prune_cnt < pruned_cnt) {  // while arcs continue to be pruned
    last_prune_cnt = pruned_cnt;
    double bestscore = GetTheta(st);  // score must be <= theta_ to be pruned
    ssize_t bestarc = -1;
    for (size_t i = 0; i < shrink_arcs->size(); ++i) {
      const ShrinkArcStats &arc = (*shrink_arcs)[i];
      if (!arc.needed && !arc.pruned) {
        if ((*shrink_arcs)[i].shrink_score <= bestscore) {
          bestscore = (*shrink_arcs)[i].shrink_score;
          bestarc = i;  // tie goes to later arcs
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

// For transitions selected to be pruned, points them to an unconnected state
template <class Arc>
size_t NGramShrink<Arc>::PointPrunedArcs(
    const vector<ShrinkArcStats> &shrink_arcs, StateId st) {
  size_t acnt = 0, pruned_cnt = 0;
  for (MutableArcIterator<MutableFst<Arc>> aiter(GetMutableFst(), st);
       !aiter.Done(); aiter.Next()) {
    Arc arc = aiter.Value();
    if (shrink_arcs[acnt].pruned) {
      arc.nextstate = dead_state_;  // points to unconnected state
      aiter.SetValue(arc);
      // decrements backoff counter
      --BackedOffTo(shrink_state_[st].backoff_state, arc.ilabel,
                    shrink_arcs[acnt].backoff_dest);
      ++pruned_cnt;
    }
    ++acnt;
  }
  // if st is a final state and the final cost is marked to be pruned, prune
  if (acnt < shrink_arcs.size() && shrink_arcs[acnt].pruned) {
    --shrink_state_[GetBackoff(st, 0)].incoming_st_back_off;
    GetMutableFst()->SetFinal(st, Arc::Weight::Zero());
    ++pruned_cnt;
  }
  return pruned_cnt;
}

// Evaluates transitions from state and prune in greedy fashion.
template <class Arc>
void NGramShrink<Arc>::PruneState(StateId st) {
  std::vector<ShrinkArcStats> shrink_arcs;
  size_t candidate_prune = FillShrinkArcInfo(&shrink_arcs, st, false);
  if (Error()) return;
  size_t pruned_cnt = ChooseArcsToPrune(&shrink_arcs, st);
  if (pruned_cnt > 0) {
    size_t check_cnt = PointPrunedArcs(shrink_arcs, st);
    if (pruned_cnt != check_cnt) {
      NGRAMERROR() << "NGramShrink: Selected arcs and pruned arcs don't match";
      NGramModel<Arc>::SetError();
      return;
    }

    if (pruned_cnt == candidate_prune)      // all candidate arcs pruned
      shrink_state_[st].state_dead = true;  // state becomes a dead state
  }
}

// Finds unpruned arcs pointing to unconnected states and points them elsewhere.
template <class Arc>
void NGramShrink<Arc>::PointArcsAwayFromDead() {
  for (StateId st = 0; st < ns_; ++st) {
    if (shrink_state_[st].state_dead) continue;
    for (MutableArcIterator<MutableFst<Arc>> aiter(GetMutableFst(), st);
         !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
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

// Maps backoff arcs of dead states to dead_state_ (except for start state).
template <class Arc>
void NGramShrink<Arc>::PointDeadBackoffArcs() {
  for (StateId st = 0; st < ns_; ++st) {
    if (!shrink_state_[st].state_dead || st == GetFst().Start()) continue;
    MutableArcIterator<MutableFst<Arc>> aiter(GetMutableFst(), st);
    if (FindMutableArc(&aiter, BackoffLabel())) {
      Arc arc = aiter.Value();
      arc.nextstate = dead_state_;
      aiter.SetValue(arc);
    } else {
      NGRAMERROR() << "NGramShrink: No backoff arc in dead state";
      NGramModel<Arc>::SetError();
      return;
    }
  }
}

// Makes model from NGram model FST with StdArc counts.
bool NGramShrinkModel(fst::StdMutableFst *fst, const string &method,
                      double tot_uni = -1.0, double theta = 0.0,
                      int64 target_num = -1, const string &count_pattern = "",
                      const string &context_pattern = "", int shrink_opt = 0,
                      fst::StdArc::Label backoff_label = 0,
                      double norm_eps = kNormEps,
                      bool check_consistency = false);

}  // namespace ngram

#endif  // NGRAM_NGRAM_SHRINK_H_
