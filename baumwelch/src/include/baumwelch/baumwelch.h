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

#ifndef BAUMWELCH_BAUMWELCH_H_
#define BAUMWELCH_BAUMWELCH_H_

// Class for storing the state of an expectation maximization training problem.
//
// In this model, the weights of a "channel" FST (a discrete-alphabet HMM) are
// optimized to maximize the joint probability with respect to some other data
// (also represented as one or more FSTs).
//
// The training data consist of one an LM-like input FST and one more more
// output FSTs. This is useful for "unsupervised" training (what Kevin Knight
// and colleagues call "decipherment") in the sense that it will learn a
// conditional probability model that maximizes the probability of the outputs
// with respect to the LM.
//
// A Likelihood() method returns the global likelihood on the previous
// iteration. Ciphertext FSTs are provided to the model using multiple
// invocations of CollectExpectations(), and the Maximize() method writes the
// collected counts back into the channel model FST. The Train() method calls
// these two steps repeatedly.
//
// Arcs not traversed during training are assigned a semiring Zero weight and
// these arcs can be removed using the RemoveZeroArcs() method. Assuming the
// training corpus is fixed, this need only be applied after the first
// iteration.
//
// By convention, operations in this library that work with FarReader input
// reset the FAR to its initial position upon completion.
//
// For an easier-to-use interface, see the functions in train.h.

#include <cmath>
#include <cstdlib>
#include <utility>
#include <vector>

#include <fst/types.h>
#include <fst/log.h>
#include <fst/extensions/far/far.h>
#include <fst/arcsort.h>
#include <fst/compose.h>
#include <fst/fst.h>
#include <baumwelch/data.h>
#include <baumwelch/expectation-table.h>
#include <baumwelch/forward-backward.h>

namespace fst {

constexpr int32 kMaxIters = 50;
constexpr int32 kRandomStarts = 10;

enum ExpectationTableType { GLOBAL, STATE, ILABEL, STATE_ILABEL };

// Struct for training options.
struct BaumWelchTrainOptions {
  explicit BaumWelchTrainOptions(
      int32 max_iters = kMaxIters,
      bool flat_start = true,
      int32 random_starts = kRandomStarts,
      bool remove_zero_arcs = true,
      float delta = kDelta,
      const CacheOptions &co_cache_options = CacheOptions(),
      const CacheOptions &ico_cache_options = CacheOptions())
      : max_iters(max_iters),
        flat_start(flat_start),
        random_starts(random_starts),
        remove_zero_arcs(remove_zero_arcs),
        delta(delta),
        co_cache_options(co_cache_options),
        ico_cache_options(ico_cache_options) {}

  // Maximum number of iterations to perform; if 0, the channel is just
  // normalized and no other work is done.
  int32 max_iters;
  // Perform one round of flat start training (i.e., as the 0th "restart")?
  bool flat_start;
  // Number of random starts to perform; if 0, random start training is not
  // performed.
  int32 random_starts;
  // Should zero-weight arcs in the cascade be removed from the channel model
  // before training?
  bool remove_zero_arcs;
  // Comparison/quantization delta used to determine convergence.
  float delta;
  // Cache options for the cascades.
  CacheOptions co_cache_options;
  CacheOptions ico_cache_options;
};

namespace internal {

// Random weight generator in the (real) interval (0, 1].
template <class Weight>
class LogUniformGenerator {
 public:
  using ValueType = typename Weight::ValueType;

  LogUniformGenerator() = default;

  // This is inclusive on the lower bound but exclusive on the upper bound.
  // We want the opposite (inclusive on the upper bound by exclusive on the
  // lower bound) so we map any sample right at the lower bound onto the
  // upper bound.
  Weight operator()() const {
    const auto sample = static_cast<ValueType>(rand()) / RAND_MAX;  // NOLINT
    return -(sample == 0.0 ? 0.0 /* i.e., log(1) */ : std::log(sample));
  }
};

// Weigh comparison function that doesn't depend on NaturalLess.
template <class Weight>
bool FloatLess(const Weight &x, const Weight &y) {
  return x.Value() > y.Value();
}

}  // namespace internal

// A class representing a Baum-Welch model and its intermediate state.
template <class Arc, class Data, class ExpectationTable>
class BaumWelch {
 public:
  using Weight = typename Arc::Weight;
  using Cascade = typename ExpectationTable::Cascade;

  explicit BaumWelch(const Fst<Arc> &channel)
      : channel_(channel),
        etable_(channel_) {
    static const OLabelCompare<Arc> comp;
    ArcSort(&channel_, comp);
  }

  BaumWelch &operator=(const BaumWelch &other) {
    channel_ = other.channel_;
    // NB: This doesn't really copy the expectations, just the table sizing.
    etable_ = other.etable_;
    return *this;
  }

  // Training.

  // E-step; called after Reset; returns the number of composition failures.
  int CollectExpectations(
      Data *data,
      const BaumWelchTrainOptions &opts = BaumWelchTrainOptions()) {
    int composition_failures = 0;
    for (; !data->Done(); data->Next()) {
      const Cascade cascade(data->GetInput(), data->GetOutput(), channel_,
                            opts.co_cache_options, opts.ico_cache_options);
      composition_failures += !Observation(cascade);
    }
    data->Reset();
    return composition_failures;
  }

  // M-step; called after CollectExpectations.
  void Maximize() {
    const auto props = ChannelLabelSorted();
    for (StateIterator<VectorFst<Arc>> siter(channel_); !siter.Done();
         siter.Next()) {
      const auto state = siter.Value();
      // Sets new arc weights.
      for (MutableArcIterator<VectorFst<Arc>> aiter(&channel_, state);
           !aiter.Done(); aiter.Next()) {
        auto arc = aiter.Value();
        arc.weight = etable_.Maximize(state, arc);
        aiter.SetValue(arc);
      }
      // Sets new final weights.
      channel_.SetFinal(state, etable_.Maximize(state));
    }
    SetChannelLabelSorted(props);
  }

  // EM iteration: reset, collect expectations, and maximize; returns the
  // number of composition failures.
  int Iteration(Data *data,
                 const BaumWelchTrainOptions &opts = BaumWelchTrainOptions()) {
    Reset();
    const int composition_failures = CollectExpectations(data, opts);
    Maximize();
    return composition_failures;
  }

  // Normalizes model by accumulating its expectations in the expectation
  // table, then maximizing.
  void Normalize() {
    Reset();
    for (StateIterator<VectorFst<Arc>> siter(channel_); !siter.Done();
         siter.Next()) {
      const auto state = siter.Value();
      for (ArcIterator<VectorFst<Arc>> aiter(channel_, state); !aiter.Done();
           aiter.Next()) {
        etable_.CollectExpectation(state, aiter.Value());
      }
      etable_.CollectExpectation(state, channel_.Final(state));
    }
    Maximize();
  }

  // Round of multi-iteration training.
  bool Train(Data *data,
             const BaumWelchTrainOptions &opts = BaumWelchTrainOptions()) {
    if (opts.remove_zero_arcs) {
      // This requires an iteration of training.
      LOG(INFO) << "Removing zero arcs";
      Iteration(data, opts);
      RemoveZeroArcs();
    }
    if (opts.max_iters < 1) return false;
    LOG(INFO) << "Normalizing model";
    Normalize();
    // First iteration requires some care so we have a likelihood to compare
    // against; in the case we aren't doing a flat start, we perform one
    // iteration of random training instead.
    bool converged = false;
    int32 random_start = 0;
    if (opts.flat_start) {
      LOG(INFO) << "Training (flat start)";
      converged = TrainFromHere(data, opts);
    } else if (opts.random_starts) {
      RandomizeArcWeights();
      LOG(INFO) << "Training (random start 1)";
      converged = TrainFromHere(data, opts);
      ++random_start;
    }
    while (random_start < opts.random_starts) {
      ++random_start;
      LOG(INFO) << "Training (random start " << random_start << ")";
      BaumWelch<Arc, Data, ExpectationTable> random(*this);
      random.RandomizeArcWeights();
      const bool random_converged = random.TrainFromHere(data, opts);
      if (internal::FloatLess(Likelihood(), random.Likelihood())) {
        LOG(INFO) << "New best model (old likelihood: " << Likelihood()
                  << "; new likelihood: " << random.Likelihood() << ")";
        *this = random;
        converged = random_converged;
      }
    }
    LOG(INFO) << "Best likelihood: " << Likelihood();
    return converged;
  }

  // Likelihood; called after Iteration or Train.
  Weight Likelihood() const { return etable_.Likelihood(); }

  // Channel access: called after Iteration or Train.
  const VectorFst<Arc> &Channel() const { return channel_; }

  // Between-iteration helpers.

  // Called at the start of each iteration.
  void Reset() { etable_.Reset(); }

  // Removes Zero-weight arcs; called after Iteration.
  // Note that this does not normalize the resulting model.
  void RemoveZeroArcs() {
    bool mutation = false;
    const auto props = ChannelLabelSorted();
    const auto dead_state = channel_.NumStates();
    for (StateIterator<VectorFst<Arc>> siter(channel_); !siter.Done();
         siter.Next()) {
      for (MutableArcIterator<VectorFst<Arc>> aiter(&channel_, siter.Value());
           !aiter.Done(); aiter.Next()) {
        if (aiter.Value().weight == Weight::Zero()) {
          auto arc = aiter.Value();
          arc.nextstate = dead_state;
          aiter.SetValue(arc);
          mutation = true;
        }
      }
    }
    if (mutation) {
      channel_.AddState();  // Which will have dead_state's ID.
      Connect(&channel_);
      SetChannelLabelSorted(props);
    }
  }

  // Assigns random values to arc weights; called before Iteration.
  void RandomizeArcWeights() {
    const auto props = ChannelLabelSorted();
    static const internal::LogUniformGenerator<Weight> generate;
    for (StateIterator<VectorFst<Arc>> siter(channel_); !siter.Done();
          siter.Next()) {
      const auto state = siter.Value();
      // Arcs leaving this state.
      for (MutableArcIterator<VectorFst<Arc>> aiter(&channel_, state);
           !aiter.Done(); aiter.Next()) {
        auto arc = aiter.Value();
        arc.weight = generate();
        aiter.SetValue(arc);
      }
      // Final weight.
      if (channel_.Final(state) != Weight::Zero()) {
        channel_.SetFinal(state, generate());
      }
    }
    Normalize();
    SetChannelLabelSorted(props);
  }

 private:
  // Single observation of the e-step.
  bool Observation(const Cascade &cascade) {
    const auto &ico = cascade.GetFst();
    const internal::ForwardBackward<Arc> fb(ico);
    const auto start = ico.Start();
    if (start == kNoStateId) {
      VLOG(1) << "Empty lattice";
      return false;
    }
    const auto &likelihood = fb.Beta(start);
    if (likelihood == Weight::Zero()) {
      VLOG(1) << "Start state not coaccessible";
      return false;
    }
    etable_.CollectLikelihood(likelihood);
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
        const auto value = Divide(Times(Times(alpha, arc.weight), beta),
                                  likelihood);
        const auto ch_nextstate = cascade.ChannelState(arc.nextstate);
        etable_.CollectExpectation(ch_state, arc.ilabel, arc.olabel, value,
                                   ch_nextstate);
      }
      const auto weight = ico.Final(state);
      if (weight == Weight::Zero()) continue;
      // The final state expectation is the product of the current weight and
      // alpha, divided by the overall observation likelihood.
      const auto value = Divide(Times(alpha, weight), likelihood);
      etable_.CollectExpectation(ch_state, value);
    }
    return true;
  }

  // Multiple iterations of training from the current state.
  bool TrainFromHere(
      Data *data, const BaumWelchTrainOptions &opts = BaumWelchTrainOptions()) {
    if (opts.max_iters < 1) return false;
    int composition_failures = Iteration(data, opts);
    auto old_likelihood = Likelihood();
    int32 iter = 1;
    LOG(INFO) << "Iteration " << iter << " likelihood: " << old_likelihood
              << "; Composition failures: " << composition_failures;
    while (iter < opts.max_iters) {
      ++iter;
      composition_failures = Iteration(data, opts);
      const auto likelihood = Likelihood();
      LOG(INFO) << "Iteration " << iter << " likelihood: " << likelihood
                << "; Composition failures: " << composition_failures;
      if (ApproxEqual(old_likelihood, likelihood, opts.delta)) {
        LOG(INFO) << "Absolute convergence";
        return true;
      } else if (internal::FloatLess(likelihood, old_likelihood)) {
        LOG(INFO) << "Approximate convergence";
        return true;
      }
      old_likelihood = likelihood;
    }
    LOG(INFO) << "Did not converge (" << opts.max_iters << " iterations)";
    return false;
  }

  // Property helpers.

  static constexpr auto kLabelSorted = kILabelSorted | kOLabelSorted;

  // Returns known label sorting properties of channel_.
  uint64 ChannelLabelSorted() {
    return channel_.Properties(kLabelSorted, false);
  }

  // Asserts that a mutation of channel_ is irrelevant to label sorting.
  void SetChannelLabelSorted(uint64 props) {
    channel_.SetProperties(props, kLabelSorted);
  }

  VectorFst<Arc> channel_;
  ExpectationTable etable_;
};

}  // namespace fst

#endif  // BAUMWELCH_BAUMWELCH_H_

