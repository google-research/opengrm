
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
// Copyright 2005-2016 Brian Roark and Google, Inc.
// NGram model class for marginalizing the model.

#ifndef NGRAM_NGRAM_MARGINALIZE_H_
#define NGRAM_NGRAM_MARGINALIZE_H_

#include <unordered_map>
using std::unordered_map;
using std::unordered_multimap;

#include <ngram/ngram-mutable-model.h>
#include <ngram/util.h>

namespace ngram {

class NGramMarginal : public NGramMutableModel<StdArc> {
 public:
  typedef StdArc::StateId StateId;
  typedef StdArc::Label Label;
  typedef StdArc::Weight Weight;

  // Construct an NGramMarginal object, including an NGramModel and parameters
  explicit NGramMarginal(StdMutableFst *infst, Label backoff_label = 0,
                         double norm_eps = kNormEps, int max_bo_updates = 10,
                         bool check_consistency = false)
      : NGramMutableModel<StdArc>(infst, backoff_label, norm_eps,
                                  check_consistency),
        max_bo_updates_(max_bo_updates) {
    ns_ = infst->NumStates();
    for (StateId st = 0; st < ns_; ++st)
      marginal_stats_.push_back(MarginalStateStats());
  };

  // Marginalize n-gram model, based on initialized parameters.  Returns
  // true if no more iterations are required.
  bool MarginalizeNGramModel(vector<double> *weights, int iter, int tot) {
    if (!CheckNormalization()) {  // requires normalized model
      // Returns true to indicate that there should be no more iterations.
      NGRAMERROR() << "NGramMarginal: Model not normalized;"
                   << " Model must be normalized for this estimation method";
      NGramModel<StdArc>::SetError();
      return true;
    }
    CalculateStateProbs(weights);  // calculate p(h)
    CalculateNewWeights();         // calculate new arc weights
    RecalcBackoff();               // re-calcs backoff weights
    if (!CheckNormalization()) {   // model should be normalized
      NGRAMERROR() << "NGramMarginal: Marginalized model not fully normalized";
      NGramModel<StdArc>::SetError();
      return true;
    }
    return CheckStateProbs(weights, iter, tot);  // check convergence
  }

 private:
  struct MarginalStateStats {
    double log_prob;         // log probability of history represented by state
    double sum_ho_log_prob;  // log sum prob of hi-order states backing off
    double sum_ho_log_prob_w_bo;  // log sum prob including backoff weights
    vector<StateId> hi_states;    // vector of states backing off to this
    vector<double> arc_found;     // arc sums from states with arc
    vector<double> arc_notfound;  // arc sums from states without arc

    MarginalStateStats()
        : log_prob(0),
          sum_ho_log_prob(-LogArc::Weight::Zero().Value()),
          sum_ho_log_prob_w_bo(-LogArc::Weight::Zero().Value()){};
  };

  // Calculates state marginal probs, and sums higher order log probs
  void CalculateStateProbs(vector<double> *weights);

  // Establish re-calculated denominator value (log_prob is minimum)
  double SaneDenominator(double total_wbo, double found_sum, double log_prob) {
    double minval = fmax(-33, log_prob),
           ret = (found_sum > total_wbo) ? -NegLogDiff(total_wbo, found_sum)
                                         : minval;
    return fmax(ret, minval);
  }

  // Add value to vector of probabilities of arcs found at higher states
  void AddToArcFound(StateId st, size_t idx, double val) {
    marginal_stats_[st].arc_found[idx] =
        -NegLogSum(-marginal_stats_[st].arc_found[idx], val);
  }

  // Add value to vector of probabilities of arcs not found at higher states
  void AddToArcNotFound(StateId st, size_t idx, double val) {
    marginal_stats_[st].arc_notfound[idx] =
        -NegLogSum(-marginal_stats_[st].arc_notfound[idx], val);
  }

  // Subt value from vector of probabilities of arcs not found at higher states
  void SubtFromArcNotFound(StateId st, size_t idx, double val) {
    marginal_stats_[st].arc_notfound[idx] =
        SaneDenominator(-marginal_stats_[st].arc_notfound[idx], val,
                        marginal_stats_[st].log_prob);
  }

  // Recalculate state probs; if different, set iteration bool
  bool CheckStateProbs(vector<double> *weights, int iter, int tot) {
    if (iter >= tot) return true;  // already the parameterized iterations
    vector<double> new_weights;    // to hold prior weights
    NGramModel::CalculateStateProbs(&new_weights, true);
    if (new_weights.size() != weights->size()) {
      // Returns true to indicate that there should be no more iterations.
      NGRAMERROR() << "Different numbers of states with steady state probs";
      NGramModel<StdArc>::SetError();
      return true;
    }
    int changed = 0;
    for (size_t st = 0; st < new_weights.size(); st++) {
      if (fabs(new_weights[st] - (*weights)[st]) >=
          fmax(kFloatEps, NGramModel::NormEps() * (*weights)[st]))
        ++changed;
      (*weights)[st] = new_weights[st];
    }
    if (changed > 0) {
      VLOG(1) << "NGramMarginal::CheckStateProbs: state probs changed: "
              << changed << " (iteration " << iter << ")";
      return false;
    }
    return true;
  }

  // Initialize every arc 'not there' value with total; or reset index to -1
  void SetArcIndices(StateId st, bool initialize);

  // Get idx from current state
  int GetCurrentArcIndex(int label);

  // Function to set accumulators based on whether arc was found or not
  // arc_found accumulates the second term in numerator of formula
  // arc_notfound accumulates the denominator of formula
  void UpdateAccum(StateId st, StateId bst, size_t idx, size_t hidx,
                   bool update_found, double arcvalue, double bo_weight);

  // Scan through arcs in a state and collect higher order statistics
  void HigherOrderArcSum(StateId st, bool sum_found);

  // Scan through states backing off to state and collect statistics
  void HigherOrderStateSum(StateId st);

  // Calculate arc weight while ensuring resulting value is sane
  double SaneArcWeight(StateId st, size_t idx, double prob, double minterm);

  // Use statistics to determine arc weights
  double GetSaneArcWeights(StateId st, vector<double> *wts);

  // Set arc weights with gathered weights
  void SetSaneArcWeights(StateId st, vector<double> *wts, double norm);

  // Use statistics to determine arc weights, second time around (new backoffs)
  // Add in formerly subtracted denominator, subtract updated denominator
  double UpdSaneArcWeights(StateId st, vector<double> *wts,
                           vector<double> *hold_notfound);

  // Recalculate backoff weights of all higher order states; indicate if changed
  bool StateHigherOrderBackoffRecalc(StateId st);

  // Recalculate backoff weights of higher order states;
  // if updated, recalculate arcs based on this.
  bool HigherOrderBackoffRecalc(StateId st, vector<double> *wts, double *norm);

  // Calculate new weights enforcing marginalization constraints
  // P(w, h') - sum_of_found gamma(w | h) p(h) / sum_of_not alpha_h p(h)
  void CalculateNewStateWeights(StateId st);

  // Calculate state weights from lowest order to highest order
  void CalculateNewWeights();

  StateId ns_;
  vector<MarginalStateStats> marginal_stats_;
  vector<int> indices_;
  int max_bo_updates_;
};

}  // namespace ngram

#endif  // NGRAM_NGRAM_MARGINALIZE_H_
