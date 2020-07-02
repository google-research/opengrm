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
// Copyright 2017 and onwards Google, Inc.

#ifndef BAUMWELCH_TRAIN_H_
#define BAUMWELCH_TRAIN_H_

#include <type_traits>
#include <utility>
#include <vector>

#include <fst/types.h>
#include <fst/log.h>
#include <fst/extensions/far/far.h>
#include <fst/compose.h>
#include <fst/fst.h>
#include <fst/shortest-distance.h>
#include <baumwelch/cascade.h>
#include <baumwelch/expectation-table.h>
#include <baumwelch/log-adder.h>
#include <baumwelch/util.h>

namespace fst {

// Some defaults.
constexpr float kLr = 1.;
constexpr int kMaxIters = 50;

// Helper for all the options. If batch_size is 0, or larger than the data,
// full-batch training is performed.
struct TrainBaumWelchOptions {
  explicit TrainBaumWelchOptions(int max_iters = kMaxIters, float lr = kLr,
                                 int batch_size = 0, float delta = kDelta,
                                 const CascadeOptions &copts = CascadeOptions())
      : max_iters(max_iters),
        lr(lr),
        batch_size(batch_size),
        delta(delta),
        copts(copts) {}

  // Maximum number of iterations to perform.
  int max_iters;
  // Learning rate.
  float lr;
  // Maximum size of a batch.
  int batch_size;
  // Comparison/quantization delta used to determine convergence.
  float delta;
  // Options passed to the trainer.
  CascadeOptions copts;
};

namespace internal {

// Computes alpha and beta for idempotent semirings, using A* search during the
// alpha computation. If a state is not visited during search the estimate is
// taken to be semiring zero. This estimate of alpha for a state has the true
// value as an upper bound, since some states not visited during the search will
// have true non-zero values because search terminates once the shortest path
// is found (due to the first_path=true).
template <class Arc, typename std::enable_if<IsIdempotent<
                         typename Arc::Weight>::value>::type * = nullptr>
void ComputeAlphaAndBeta(const ComposeFst<Arc> &ico,
                         std::vector<typename Arc::Weight> *alpha,
                         std::vector<typename Arc::Weight> *beta) {
  // Computes beta.
  ShortestDistance(ico, beta, /*reverse=*/true);
  // Computes alpha using an A* approximation.
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  using MyEstimate = NaturalAStarEstimate<StateId, Weight>;
  using MyQueue = NaturalAStarQueue<StateId, Weight, MyEstimate>;
  using MyArcFilter = AnyArcFilter<Arc>;
  using MyShortestDistanceOptions =
      ShortestDistanceOptions<Arc, MyQueue, MyArcFilter>;
  const MyEstimate estimate(*beta);
  MyQueue queue(*alpha, estimate);
  static constexpr MyArcFilter arc_filter;
  const MyShortestDistanceOptions opts(
      &queue, arc_filter,
      /*source=*/kNoStateId,     // Default.
      /*delta=*/kShortestDelta,  // Default.
      /*first_path=*/true);      // Heuristic is admissible.
  ShortestDistance(ico, alpha, opts);
  VLOG(1) << ExploredStates<Weight>(*alpha) << " alpha states explored";
}

// Computes alpha and beta for non-idempotent semirings, which requires full
// search.
template <class Arc, typename std::enable_if<!IsIdempotent<
                         typename Arc::Weight>::value>::type * = nullptr>
void ComputeAlphaAndBeta(const ComposeFst<Arc> &ico,
                         std::vector<typename Arc::Weight> *alpha,
                         std::vector<typename Arc::Weight> *beta) {
  // Computes beta.
  ShortestDistance(ico, beta, /*reverse=*/true);
  // Computes alpha.
  ShortestDistance(ico, alpha, /*reverse=*/false);
}

// Class storing forward and backwards weights
template <class Arc>
class ForwardBackward {
 public:
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  explicit ForwardBackward(const ComposeFst<Arc> &ico) {
    ComputeAlphaAndBeta(ico, &alpha_, &beta_);
  }

  const Weight &Alpha(StateId s) const {
    return ForwardBackward::WeightOrZero(s, alpha_);
  }

  const Weight &Beta(StateId s) const {
    return ForwardBackward::WeightOrZero(s, beta_);
  }

 private:
  static constexpr Weight kZero = Weight::Zero();

  // Returns the shortest distance weight, or semiring zero if the state was
  // not visited during the respective shortest distance computation.
  static const Weight &WeightOrZero(StateId s,
                                    const std::vector<Weight> &weights) {
    return (s < weights.size()) ? weights[s] : kZero;
  }

  std::vector<Weight> alpha_;
  std::vector<Weight> beta_;
};

template <class Arc>
constexpr typename ForwardBackward<Arc>::Weight ForwardBackward<Arc>::kZero;

// Object which holds all necessary information for stepwise or minibatch
// training. It stores the (initial) learning rate and the step counter. For
// more information, see the "sEM" pseudocode (p. 613) in:
//
// Liang, P., and Klein, D. 2009. Online EM for unsupervised models. In
// NAACL, pages 611-619.
template <class Arc, class ExpectationTable>
class StepwiseBaumWelchTrainer {
 public:
  using Weight = typename Arc::Weight;
  using Sum = LogAdder<Weight>;

  // Valid values of alpha are usually between [.5, 1.0].
  explicit StepwiseBaumWelchTrainer(
      float lr = kLr, int batch_size = 0,
      const CascadeOptions &opts = CascadeOptions())
      : lr_(lr), batch_size_(batch_size), opts_(opts), step_(0) {}

  // The Step methods all perform a single step or minibatch of stepwise
  // training.

  // Performs a batch of training returning the likelihood. Semiring Zero is
  // returned in the case of composition failure.
  Weight Step(FarReader<Arc> *input, FarReader<Arc> *output,
              MutableFst<Arc> *model) {
    ExpectationTable table(*model);
    Sum likelihood;  // Tracks batch likelihood.
    for (int batch_step = 0; !input->Done() && !output->Done() &&
                             (!batch_size_ || batch_step < batch_size_);
         ++batch_step) {
      likelihood.Add(
          Forward(*input->GetFst(), *output->GetFst(), *model, &table));
      if (input->Type() != FarType::FST) input->Next();
      output->Next();
    }
    Backward(table, model);
    ++step_;
    return likelihood.Sum();
  }

  // The Train methods repeatedly call Step methods and are usually used to
  // perform a single pass over the data.

  // Repeatedly do the stepwise computation.
  Weight Train(FarReader<Arc> *input, FarReader<Arc> *output,
               MutableFst<Arc> *model) {
    Sum likelihood;  // Tracks iteration likelihood.
    while (!input->Done() && !output->Done()) {
      likelihood.Add(Step(input, output, model));
    }
    return likelihood.Sum();
  }

 private:
  Weight Forward(const Fst<Arc> &input, const Fst<Arc> &output,
                 const Fst<Arc> &model, ExpectationTable *table) {
    const ChannelStateCascade<Arc> cascade(input, output, model, opts_);
    const auto &ico = cascade.GetFst();
    const auto start = ico.Start();
    if (start == kNoStateId) {
      VLOG(1) << "Empty lattice";
      return false;
    }
    const internal::ForwardBackward<Arc> fb(ico);
    const auto &likelihood = fb.Beta(start);
    if (likelihood == Weight::Zero()) {
      VLOG(1) << "Start state not coaccessible";
      return Weight::Zero();
    }
    for (StateIterator<ComposeFst<Arc>> siter(ico); !siter.Done();
         siter.Next()) {
      const auto state = siter.Value();
      // Non-coaccessible source state.
      if (fb.Beta(state) == Weight::Zero()) continue;
      const auto ch_state = cascade.ChannelState(state);
      const auto &alpha = fb.Alpha(state);
      for (ArcIterator<ComposeFst<Arc>> aiter(ico, state); !aiter.Done();
           aiter.Next()) {
        const auto &arc = aiter.Value();
        const auto &beta = fb.Beta(arc.nextstate);
        // Non-coaccessible destination state.
        if (beta == Weight::Zero()) continue;
        // The arc expectation is the product of the current weight, alpha,
        // and beta, divided by the overall observation likelihood.
        table->Forward(
            ch_state, arc.ilabel, arc.olabel,
            Divide(Times(Times(alpha, arc.weight), beta), likelihood),
            cascade.ChannelState(arc.nextstate));
      }
      const auto weight = ico.Final(state);
      if (weight == Weight::Zero()) continue;
      // The final state expectation is the product of the current weight and
      // alpha, divided by the overall observation likelihood.
      table->Forward(ch_state, Divide(Times(alpha, weight), likelihood));
    }
    return likelihood;
  }

  // TODO(kbg): Add a way to disable interpolation.
  static Weight Interpolate(const Weight &old_weight, const Weight &new_weight,
                            double nu_k) {
    const auto old_term = Times(1 - nu_k, old_weight);
    const auto new_term = Times(nu_k, new_weight);
    Sum plus(old_term);
    plus.Add(new_term);
    return plus.Sum();
  }

  void Backward(const ExpectationTable &table, MutableFst<Arc> *model) {
    const double nu_k = std::pow(step_ + 2, -lr_);
    for (StateIterator<MutableFst<Arc>> siter(*model); !siter.Done();
         siter.Next()) {
      const auto state = siter.Value();
      // Sets new arc weights.
      for (MutableArcIterator<MutableFst<Arc>> aiter(model, state);
           !aiter.Done(); aiter.Next()) {
        auto arc = aiter.Value();
        arc.weight = Interpolate(arc.weight, table.Backward(state, arc), nu_k);
        aiter.SetValue(arc);
      }
      // Sets new final weights.
      model->SetFinal(
          state, Interpolate(model->Final(state), table.Backward(state), nu_k));
    }
  }

  const float lr_;        // Learning rate hypparameter.
  const int batch_size_;  // Batch size hyperparameter.
  const CascadeOptions opts_;
  uint64 step_;  // Iteration/step number.
};

// Full training setup, templated on expectation table.
template <class Arc, class ExpectationTable>
typename Arc::Weight TrainBaumWelch(
    FarReader<Arc> *input, FarReader<Arc> *output, MutableFst<Arc> *model,
    const TrainBaumWelchOptions &opts = TrainBaumWelchOptions()) {
  using Weight = typename Arc::Weight;
  auto last_likelihood = Weight::Zero();
  StepwiseBaumWelchTrainer<Arc, ExpectationTable> trainer(
      opts.lr, opts.batch_size, opts.copts);
  for (int iter = 1; iter <= opts.max_iters; ++iter) {
    input->Reset();
    output->Reset();
    const auto likelihood = trainer.Train(input, output, model);
    LOG(INFO) << "Iteration " << iter << ": " << likelihood;
    if (ApproxEqual(last_likelihood, likelihood, opts.delta)) return likelihood;
    last_likelihood = likelihood;
  }
  return last_likelihood;
}

}  // namespace internal

// Full training setup.
template <class Arc>
typename Arc::Weight TrainBaumWelch(
    FarReader<Arc> *input, FarReader<Arc> *output, MutableFst<Arc> *model,
    bool normalize_ilabel = true,
    const TrainBaumWelchOptions &opts = TrainBaumWelchOptions()) {
  if (normalize_ilabel) {
    return internal::TrainBaumWelch<Arc, StateILabelExpectationTable<Arc>>(
        input, output, model, opts);
  } else {
    return internal::TrainBaumWelch<Arc, StateExpectationTable<Arc>>(
        input, output, model, opts);
  }
}

}  // namespace fst

#endif  // BAUMWELCH_TRAIN_H_

