// Copyright 2026 The OpenGrm Authors.
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

#ifndef OPENGRM_BAUMWELCH_TRAIN_H_
#define OPENGRM_BAUMWELCH_TRAIN_H_

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "openfst/extensions/far/far.h"
#include "openfst/lib/arcfilter.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/queue.h"
#include "openfst/lib/shortest-distance.h"
#include "openfst/lib/shortest-path.h"
#include "openfst/lib/weight.h"
#include "opengrm/baumwelch/cascade.h"
#include "opengrm/baumwelch/expectation-table.h"
#include "opengrm/baumwelch/log-adder.h"
#include "opengrm/baumwelch/util.h"

namespace fst {

// Some defaults.
constexpr float kAlpha = 1.;
constexpr int kMaxIters = 50;

// Helper for training options. If batch_size is 0, or larger than the data,
// full-batch training is performed.
struct TrainOptions {
  explicit TrainOptions(int max_iters = kMaxIters, float alpha = kAlpha,
                        int batch_size = 0, float delta = kDelta,
                        const CascadeOptions& copts = CascadeOptions())
      : max_iters(max_iters),
        alpha(alpha),
        batch_size(alpha == 0.0 ? 0 : batch_size),
        delta(delta),
        copts(copts) {}

  // Maximum number of iterations to perform.
  int max_iters;
  // Step size reduction power. When non-zero, step size is (k + 2)^{-alpha},
  // where k is the step.
  float alpha;
  // Maximum size of a batch.  If set to 0, full-batch training occurs.
  int batch_size;
  // Comparison/quantization delta used to determine convergence.
  float delta;
  // Options passed to the trainer.
  CascadeOptions copts;
};

namespace internal {

// Class storing forward and backwards weights for non-idempotent semirings
// (e.g., LogWeight), where Baum-Welch EM is performed using Bellman-Ford
// to compute alpha and beta scores.
template <class Arc>
class ForwardBackward {
 public:
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  explicit ForwardBackward(const ComposeFst<Arc>& ico) {
    ShortestDistance(ico, &beta_, /*reverse=*/true);
    ShortestDistance(ico, &alpha_, /*reverse=*/false);
  }

  const Weight& Alpha(StateId s) const {
    return ForwardBackward::WeightOrZero(s, alpha_);
  }

  const Weight& Beta(StateId s) const {
    return ForwardBackward::WeightOrZero(s, beta_);
  }

 private:
  static constexpr Weight kZero = Weight::Zero();

  static const Weight& WeightOrZero(StateId s,
                                    const std::vector<Weight>& weights) {
    return (s < weights.size()) ? weights[s] : kZero;
  }

  std::vector<Weight> alpha_;
  std::vector<Weight> beta_;
};

// Object which holds all necessary information for stepwise or minibatch
// training. It stores the (initial) learning rate and the step counter. For
// more information, see the "sEM" pseudocode (p. 613) in:
//
// Liang, P., and Klein, D. 2009. Online EM for unsupervised models. In
// Proceedings of Human Language Technologies: The 2009 Annual Conference of
// the North American Chapter of the Association for Computational Linguistics,
// pages 611-619.
template <class Arc, class ExpectationTable>
class Trainer {
 public:
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  using Sum = LogAdder<Weight>;

  explicit Trainer(float alpha = kAlpha, int batch_size = 0,
                   float delta = kDelta,
                   const CascadeOptions& opts = CascadeOptions())
      : alpha_(alpha),
        batch_size_(batch_size),
        delta_(delta),
        opts_(opts),
        step_(0) {}

  virtual ~Trainer() = default;

  // Performs a batch of training returning the likelihood. Semiring Zero is
  // returned in the case of composition failure.
  Weight Batch(FarReader<Arc>& input, FarReader<Arc>& output,
               MutableFst<Arc>* model) {
    ExpectationTable table(*model);
    Sum likelihood;     // Tracks batch likelihood.
    int batch_idx = 0;  // Tracks actual batch size.
    for (; !input.Done() && !output.Done() &&
           (!batch_size_ || batch_idx < batch_size_);
         ++batch_idx) {
      likelihood.Add(
          Forward(*input.GetFst(), *output.GetFst(), *model, &table));
      if (input.Type() != FarType::FST) input.Next();
      output.Next();
    }
    Backward(table, model);
    ++step_;
    const auto batch_likelihood = likelihood.Sum();
    LOG(INFO) << "Step " << step_ << " (batch size " << batch_idx
              << ") average likelihood: "
              << batch_likelihood.Value() / batch_idx;
    return batch_likelihood;
  }

  // Repeatedly do the stepwise computation.
  Weight Train(FarReader<Arc>& input, FarReader<Arc>& output,
               MutableFst<Arc>* model) {
    Sum likelihood;  // Tracks iteration likelihood.
    while (!input.Done() && !output.Done()) {
      likelihood.Add(Batch(input, output, model));
    }
    Normalize(model);
    return likelihood.Sum();
  }

  // Normalizes the model.
  static void Normalize(MutableFst<Arc>* model) {
    ExpectationTable table(*model);
    StateIterator<MutableFst<Arc>> siter(*model);
    for (; !siter.Done(); siter.Next()) {
      const auto state = siter.Value();
      for (ArcIterator<MutableFst<Arc>> aiter(*model, state); !aiter.Done();
           aiter.Next()) {
        const auto& arc = aiter.Value();
        table.Forward(state, arc.ilabel, arc.olabel, arc.weight, arc.nextstate);
      }
      const auto weight = model->Final(state);
      if (weight == Weight::Zero()) continue;
      table.Forward(state, weight);
    }
    for (siter.Reset(); !siter.Done(); siter.Next()) {
      const auto state = siter.Value();
      for (MutableArcIterator<MutableFst<Arc>> aiter(model, state);
           !aiter.Done(); aiter.Next()) {
        auto arc = aiter.Value();
        arc.weight = table.Backward(state, arc);
        aiter.SetValue(arc);
      }
      model->SetFinal(state, table.Backward(state));
    }
  }

 protected:
  static Weight Interpolate(const Weight& old_weight, const Weight& new_weight,
                            double nu_k) {
    if (nu_k == 1.0) return new_weight;
    Sum plus(Times(old_weight, Weight(-std::log1p(-nu_k))));
    plus.Add(Times(new_weight, Weight(-std::log(nu_k))));
    return plus.Sum();
  }

  void Backward(const ExpectationTable& table, MutableFst<Arc>* model) {
    const double nu_k = alpha_ == 0.0 ? 1.0 : std::pow(step_ + 2, -alpha_);
    for (StateIterator<MutableFst<Arc>> siter(*model); !siter.Done();
         siter.Next()) {
      const auto state = siter.Value();
      for (MutableArcIterator<MutableFst<Arc>> aiter(model, state);
           !aiter.Done(); aiter.Next()) {
        auto arc = aiter.Value();
        arc.weight = Interpolate(arc.weight, table.Backward(state, arc), nu_k);
        aiter.SetValue(arc);
      }
      model->SetFinal(
          state, Interpolate(model->Final(state), table.Backward(state), nu_k));
    }
  }

 protected:
  const CascadeOptions& opts() const { return opts_; }

  float delta() const { return delta_; }

 private:
  virtual Weight Forward(const Fst<Arc>& input, const Fst<Arc>& output,
                         const Fst<Arc>& model, ExpectationTable* table) = 0;

  const float alpha_;
  const int batch_size_;
  const float delta_;
  const CascadeOptions opts_;
  uint64_t step_;
};

template <class Arc, class ExpectationTable>
class BaumWelchTrainer : public Trainer<Arc, ExpectationTable> {
 public:
  using Base = Trainer<Arc, ExpectationTable>;
  using Weight = typename Arc::Weight;

  using Base::Base;

 private:
  Weight Forward(const Fst<Arc>& input, const Fst<Arc>& output,
                 const Fst<Arc>& model, ExpectationTable* table) override {
    const ChannelStateCascade<Arc> cascade(input, output, model, this->opts());
    const auto& ico = cascade.GetFst();
    const auto start = ico.Start();
    if (start == kNoStateId) {
      VLOG(1) << "Empty lattice";
      return Weight::Zero();
    }
    const ForwardBackward<Arc> fb(ico);
    const auto& likelihood = fb.Beta(start);
    if (likelihood == Weight::Zero()) {
      VLOG(1) << "Start state not coaccessible";
      return Weight::Zero();
    }
    for (StateIterator<ComposeFst<Arc>> siter(ico); !siter.Done();
         siter.Next()) {
      const auto state = siter.Value();
      if (fb.Beta(state) == Weight::Zero()) continue;
      const auto ch_state = cascade.ChannelState(state);
      const auto& alpha = fb.Alpha(state);
      for (ArcIterator<ComposeFst<Arc>> aiter(ico, state); !aiter.Done();
           aiter.Next()) {
        const auto& arc = aiter.Value();
        const auto& beta = fb.Beta(arc.nextstate);
        if (beta == Weight::Zero()) continue;
        table->Forward(
            ch_state, arc.ilabel, arc.olabel,
            Divide(Times(Times(alpha, arc.weight), beta), likelihood),
            cascade.ChannelState(arc.nextstate));
      }
      const auto weight = ico.Final(state);
      if (weight == Weight::Zero()) continue;
      table->Forward(ch_state, Divide(Times(alpha, weight), likelihood));
    }
    return likelihood;
  }
};

// Object which performs Viterbi EM training for idempotent semirings
// (e.g., TropicalWeight) based on the single most-likely alignment path.
// If a state is not visited during shortest-path search, its expectation
// is taken to be semiring zero.
template <class Arc, class ExpectationTable>
class ViterbiTrainer : public Trainer<Arc, ExpectationTable> {
 public:
  using Base = Trainer<Arc, ExpectationTable>;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  using Base::Base;

 private:
  Weight Forward(const Fst<Arc>& input, const Fst<Arc>& output,
                 const Fst<Arc>& model, ExpectationTable* table) override {
    const ChannelStateCascade<Arc> cascade(input, output, model, this->opts());
    const auto& ico = cascade.GetFst();
    const auto start = ico.Start();
    if (start == kNoStateId) {
      VLOG(1) << "Empty lattice";
      return Weight::Zero();
    }
    std::vector<std::pair<StateId, size_t>> parent;
    StateId best_final_state;
    std::vector<Weight> distance;
    AnyArcFilter<Arc> arc_filter;
    AutoQueue<StateId> state_queue(ico, &distance, arc_filter);
    const ShortestPathOptions<Arc, AutoQueue<StateId>, AnyArcFilter<Arc>> sopts(
        &state_queue, arc_filter, /*nshortest=*/1, /*unique=*/false,
        /*has_distance=*/false, this->delta(), /*first_path=*/true);
    if (SingleShortestPath(ico, &distance, sopts, &best_final_state, &parent)) {
      if (best_final_state == kNoStateId) return Weight::Zero();
      const auto likelihood =
          Times(distance[best_final_state], ico.Final(best_final_state));
      for (StateId state = best_final_state, d = kNoStateId;
           state != kNoStateId; d = state, state = parent[state].first) {
        if (d != kNoStateId) {
          ArcIterator<Fst<Arc>> aiter(ico, state);
          aiter.Seek(parent[d].second);
          const auto& arc = aiter.Value();
          const auto ch_state = cascade.ChannelState(state);
          const auto next_ch_state = cascade.ChannelState(arc.nextstate);
          table->Forward(ch_state, arc.ilabel, arc.olabel, Weight::One(),
                         next_ch_state);
        }
      }
      const auto ch_state = cascade.ChannelState(best_final_state);
      table->Forward(ch_state, Weight::One());
      return likelihood;
    } else {
      return Weight::Zero();
    }
  }
};

// Full training setup, templated on expectation table.
template <class Arc, class ExpectationTable, class Trainer>
typename Arc::Weight TrainWithTrainer(FarReader<Arc>& input,
                                      FarReader<Arc>& output,
                                      MutableFst<Arc>* model,
                                      const TrainOptions& opts) {
  using Weight = typename Arc::Weight;
  auto last_likelihood = Weight::Zero();
  Trainer trainer(opts.alpha, opts.batch_size, opts.delta, opts.copts);
  trainer.Normalize(model);
  for (int iteration = 0; iteration < opts.max_iters; ++iteration) {
    input.Reset();
    output.Reset();
    const auto total_likelihood = trainer.Train(input, output, model);
    LOG(INFO) << "Iteration " << iteration + 1
              << " total likelihood: " << total_likelihood;
    if (ApproxEqual(last_likelihood, total_likelihood, opts.delta)) {
      return total_likelihood;
    }
    last_likelihood = total_likelihood;
  }
  return last_likelihood;
}

template <class Arc, class ExpectationTable>
typename Arc::Weight Train(FarReader<Arc>& input, FarReader<Arc>& output,
                           MutableFst<Arc>* model,
                           const TrainOptions& opts = TrainOptions()) {
  using Weight = typename Arc::Weight;
  if constexpr (IsIdempotent<Weight>::value) {
    return TrainWithTrainer<Arc, ExpectationTable,
                            ViterbiTrainer<Arc, ExpectationTable>>(
        input, output, model, opts);
  } else {
    return TrainWithTrainer<Arc, ExpectationTable,
                            BaumWelchTrainer<Arc, ExpectationTable>>(
        input, output, model, opts);
  }
}

}  // namespace internal

// Full training setup.
//
// This library implements stepwise expectation-maximization (EM) training for a
// WFST model given pairs of input and output FSTs. This is suitable for
// learning parameters for sequence-to-sequence tasks like G2P, transliteration,
// or pronunciation modeling, where training data consists of pairs of sequences
// that can be modeled as FSTs. If used with LogWeight, it maximizes the
// likelihood of observing the output FST given the input FST, marginalized over
// all alignments through the model:
//
// P(Output|Input) = P(Input o Model o Output).
//
// If used with TropicalWeight, it performs Viterbi EM training based on the
// most-likely alignment. Despite the name, this library does not perform
// classical Baum-Welch training for hidden Markov models (HMMs) from
// observation sequences alone; it requires input/output pairs.
template <class Arc>
typename Arc::Weight Train(FarReader<Arc>& input, FarReader<Arc>& output,
                           MutableFst<Arc>* model, bool normalize_ilabel = true,
                           const TrainOptions& opts = TrainOptions()) {
  if (normalize_ilabel) {
    return internal::Train<Arc, StateILabelExpectationTable<Arc>>(input, output,
                                                                  model, opts);
  } else {
    return internal::Train<Arc, StateExpectationTable<Arc>>(input, output,
                                                            model, opts);
  }
}

}  // namespace fst

#endif  // OPENGRM_BAUMWELCH_TRAIN_H_
