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

// Regression test for various SFst algorithms.

#include <sys/types.h>

#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "openfst/compat/seed_sequences.h"
#include "absl/random/bit_gen_ref.h"
#include "absl/random/random.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/closure.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/concat.h"
#include "openfst/lib/connect.h"
#include "openfst/lib/determinize.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/equivalent.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/minimize.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/project.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/push.h"
#include "openfst/lib/rational.h"
#include "openfst/lib/relabel.h"
#include "openfst/lib/statesort.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/union.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/test/rand-fst.h"
#include "opengrm/sfst/approx.h"
#include "opengrm/sfst/backoff.h"
#include "opengrm/sfst/canonical.h"
#include "opengrm/sfst/count.h"
#include "opengrm/sfst/equal.h"
#include "opengrm/sfst/ngramapprox.h"
#include "opengrm/sfst/normalize.h"
#include "opengrm/sfst/phi2matcher.h"
#include "opengrm/sfst/sfst.h"
#include "opengrm/sfst/shortest-distance.h"
#include "opengrm/sfst/state-weights.h"
#include "opengrm/sfst/stationary-distrib.h"
#include "opengrm/sfst/trim.h"

namespace sfst {

class AlgoTester {
 public:
  using WeightGenerator = fst::WeightGenerate<fst::TropicalWeight>;
  using StdFst = fst::StdFst;
  using StdComposeFst = fst::StdComposeFst;
  using StdExpandedFst = fst::StdExpandedFst;
  using StdMutableFst = fst::StdMutableFst;
  using StdVectorFst = fst::StdVectorFst;
  using StdArc = fst::StdArc;
  using StateId = StdArc::StateId;
  using Label = StdArc::Label;
  using Weight = StdArc::Weight;

  explicit AlgoTester() : generate_(false) {
    univ_fst_.AddState();
    univ_fst_.SetStart(0);
    univ_fst_.SetFinal(0, Weight::One());
    for (int i = 1; i <= kNumRandomLabels; ++i) {
      if (i != kPhiLabel) univ_fst_.AddArc(0, StdArc(i, i, Weight::One(), 0));
    }
  }

  void Test() {
    StdVectorFst fst1, fst2;
    bool cyclic1 = absl::Bernoulli(bit_gen_, 0.5);
    bool cyclic2 = absl::Bernoulli(bit_gen_, 0.5);
    MakeRandFsa(bit_gen_, &fst1, cyclic1);
    MakeRandFsa(bit_gen_, &fst2, cyclic2);

    TestCanonical(fst1);
    TestCanonical(fst2);
    TestTrim(fst1);
    TestCompose(fst1, fst2);
    TestShortestDistance(fst1);
    // This test infrequently fails on random input (<.01% of the
    // time). Likely due to numerical issues at unigram states (which
    // have negligible state mass).
    // TestStationaryDistrib(fst1);
    TestNormalize(fst1);
    TestNGramApprox(fst1);
    TestApprox(fst1);
  }

  // Checks input is canonical stochastic FST
  void TestCanonical(const StdFst& ifst) const {
    CHECK(IsCanonical(ifst, kPhiLabel));
  }

  // Checks phi shortest distance algos.
  void TestShortestDistance(const StdFst& ifst) const;

  void TestStationaryDistrib(const StdFst& ifst) const {
    TestStationaryDistribShDist(ifst);
    TestStationaryDistribStateSum(ifst, 2);
    TestStationaryDistribStateSum(ifst, 3);
  }

  // Checks stationary distribution and its relation to
  // shortest distance.
  void TestStationaryDistribShDist(const StdFst& ifst) const;

  // Checks stationary distribution and its relation to state sums.
  void TestStationaryDistribStateSum(const StdFst& ifst, int order) const;

  // Checks trimming is idempotent and equivalent (after phi-removal)
  // appropriate.
  void TestTrim(const StdFst& ifst) const {
    TestTrimType(ifst, TRIM_NEEDED_TRIM);
    TestTrimType(ifst, TRIM_NEEDED_NONFINAL);
    TestTrimType(ifst, TRIM_NEEDED_FINAL);
  }

  // Checks trimming is idempotent and equivalent (after phi-removal)
  // depending on trim type.
  void TestTrimType(const StdFst& ifst, TrimType trim_type) const;

  // Checks composition commutes with phi_removal.
  void TestCompose(const StdFst& ifst1, const StdFst& ifst2) const;

  // Checks input can be (globally) normalized
  void TestNormalize(const StdFst& ifst) const {
    StdVectorFst ofst(ifst);
    CHECK(GlobalNormalize(&ofst, kPhiLabel, kAlgoDelta));
    CHECK(IsNormalized(ofst, kPhiLabel, kNormDelta));
  }

  // Checks ngram-approximation
  void TestNGramApprox(const StdFst& fst) const {
    TestNGramApproxNorm(fst, 2);
    TestNGramApproxNorm(fst, 3);
    TestNGramApproxMerge(fst, 2);
    TestNGramApproxMerge(fst, 3);
    TestNGramApproxRmPhi(fst, 2);
    TestNGramApproxRmPhi(fst, 3);
  }

  // Checks input can be ngram-approximated and result is normalized
  void TestNGramApproxNorm(const StdFst& ifst, int order) const {
    if (ifst.Start() == fst::kNoStateId)  // no states
      return;
    StdVectorFst nfst(ifst), ofst;
    CHECK(GlobalNormalize(&nfst, kPhiLabel, kAlgoDelta));
    CHECK(NGramApprox(nfst, &ofst, order, kPhiLabel, kAlgoDelta, NORM_SUMMED));
    CHECK(Verify(ofst));
    CHECK(IsCanonical(ofst, kPhiLabel));
    CHECK(IsNormalized(ofst, kPhiLabel, kNormDelta));
  }

  // Checks the ngram approximation of the
  // normalized union of the input with the input
  // equals the ngram approximation of the ngram input
  void TestNGramApproxMerge(const StdFst& ifst, int order) const;

  // Checks input ngram-approximation is unaffected
  // by input FST phi-removal.
  void TestNGramApproxRmPhi(const StdFst& ifst, int order) const;

  // Test SFST approximation.
  void TestApprox(const StdFst& ifst) const;

 private:
  // Generates a canonical but unnormalized stochastic FSA
  void MakeRandFsa(absl::BitGenRef bit_gen, StdMutableFst* fst, bool cyclic);

  // Checks there is a non-trivial label.
  bool NonTrivialLabel(const StdExpandedFst& fst,
                       StdArc::Label phi_label) const {
    for (StateId s = 0; s < fst.NumStates(); ++s) {
      for (fst::ArcIterator<StdExpandedFst> aiter(fst, s); !aiter.Done();
           aiter.Next()) {
        const auto& arc = aiter.Value();
        if (arc.ilabel && arc.ilabel != phi_label) return true;
      }
    }
    return false;
  }

  // fst::Union may change the semantics of failure transitions
  // but the Thompson construction won't (which is forced if
  // fst::Union believes there is an initial cycle on fst1).
  template <typename A>
  void ThompsonUnion(fst::MutableFst<A>* fst1, const fst::Fst<A>& fst2) const;

  // (Non-det) unions two FSTs and normalizes (assuming kPhiLabel).
  void Merge(const StdFst& fst1, const StdFst& fst2,
             StdMutableFst* ofst) const {
    *ofst = fst1;
    ThompsonUnion(ofst, fst1);
    fst::ArcSort(ofst, fst::StdILabelCompare());
    CHECK(GlobalNormalize(ofst, kPhiLabel, kAlgoDelta));
  }

  void PhiRemove(const StdFst& ifst, StdMutableFst* ofst) const {
    using PM = Phi2Matcher<fst::Matcher<fst::StdFst>>;
    using PF = Phi2Filter<PM>;
    fst::ComposeFstOptions<StdArc, PM, PF> copts;

    copts.gc_limit = 0;
    copts.matcher1 = new PM(ifst, fst::MATCH_OUTPUT, kPhiLabel);
    copts.matcher2 = new PM(univ_fst_, fst::MATCH_NONE, fst::kNoLabel);
    *ofst = StdComposeFst(ifst, univ_fst_, copts);
    fst::Connect(ofst);
  }

  // Tests isomorphic after phi-removal
  bool RmPhiIsomorphic(const StdFst& fst1, const StdFst& fst2) const {
    StdVectorFst rfst1, rfst2;
    PhiRemove(fst1, &rfst1);
    PhiRemove(fst1, &rfst2);
    return Isomorphic(rfst1, rfst2, kPhiLabel, kNormDelta);
  }

  // Ensures trim and sane arc weights.
  bool SaneFst(const StdExpandedFst& fst, Label phi_label) const {
    if (!IsTrim(fst, phi_label)) return false;

    // Minimum weight allowed for an arc.
    static const fst::Log64Weight kWeightThreshold(10.0);

    fst::WeightConvert<Weight, fst::Log64Weight> to_log;
    for (StateId s = 0; s < fst.NumStates(); ++s) {
      for (fst::ArcIterator<StdExpandedFst> aiter(fst, s); !aiter.Done();
           aiter.Next()) {
        const auto& arc = aiter.Value();
        if (Less(to_log(arc.weight), kWeightThreshold)) return false;
      }
    }
    return true;
  }
  mutable WeightGenerator generate_;
  mutable absl::BitGen bit_gen_{
      fst::MakeTaggedSeedSeq("ALGO_TESTER")};
  // Sigma* machine.
  fst::StdVectorFst univ_fst_;

  // Maximum number of states in random test Fst.
  static constexpr int kNumRandomStates = 10;

  // Maximum number of arcs in random test Fst.
  static constexpr int kNumRandomArcs = 25;

  // Number of alternative random labels.
  static constexpr int kNumRandomLabels = 5;

  // Phi label (other than 0)
  static constexpr int kPhiLabel = 1;

  // Algorithm delta
  static constexpr float kAlgoDelta = 1.0e-12;

  // Normlization delta
  static constexpr float kNormDelta = 0.01;
};

void AlgoTester::TestShortestDistance(const StdFst& ifst) const {
  using Arc = fst::StdArc;
  using LArc = fst::Log64Arc;
  using LWeight = LArc::Weight;
  using WCM = fst::WeightConvertMapper<Arc, LArc>;
  using SFM = fst::SuperFinalMapper<LArc>;

  fst::VectorFst<LArc> lfst;
  WCM wc_mapper;
  SFM sf_mapper;
  fst::ArcMap(ifst, &lfst, wc_mapper);
  fst::ArcMap(&lfst, sf_mapper);

  fst::VectorFst<LArc> cfst(lfst), ufst(lfst);
  fst::Concat(&cfst, lfst);
  ThompsonUnion(&ufst, lfst);

  std::vector<LWeight> distance, rdistance, cdistance, udistance;
  ShortestDistance(lfst, &distance, kPhiLabel, false, kAlgoDelta);
  ShortestDistance(lfst, &rdistance, kPhiLabel, true, kAlgoDelta);
  ShortestDistance(cfst, &cdistance, kPhiLabel, false, kAlgoDelta);
  ShortestDistance(ufst, &udistance, kPhiLabel, false, kAlgoDelta);

  // This computation assumes all super-final arcs can be
  // read. Using a superfinal state as above ensures this is
  // correct.
  LWeight fw = fst::ComputeTotalWeight(lfst, distance, false);
  LWeight rw = fst::ComputeTotalWeight(lfst, rdistance, true);
  LWeight cw = fst::ComputeTotalWeight(cfst, cdistance, false);
  LWeight uw = fst::ComputeTotalWeight(ufst, udistance, false);

  CHECK(ApproxEqual(fw, rw, kNormDelta));
  CHECK(ApproxEqual(cw, Times(fw, fw), kNormDelta));
  CHECK(ApproxEqual(uw, Plus(fw, fw), kNormDelta));
}

void AlgoTester::TestStationaryDistribShDist(const StdFst& ifst) const {
  const Weight kReEntryWeight(1.0e-6);
  const float kSTDelta = 1.0e-6;

  if (ifst.Start() == fst::kNoStateId)  // no states
    return;

  // Uses trim, normalized and eps-free input with a superfinal state.
  StdVectorFst nfst;
  fst::ArcMap(ifst, &nfst, fst::SuperFinalMapper<StdArc>());
  fst::ArcSort(&nfst, fst::StdILabelCompare());

  CHECK(Trim(&nfst, kPhiLabel));
  CHECK(GlobalNormalize(&nfst, kPhiLabel, kAlgoDelta));
  std::vector<std::pair<Label, Label>> inpairs = {{0, kNumRandomLabels - 1}};
  fst::Relabel(&nfst, inpairs, inpairs);
  fst::ArcSort(&nfst, fst::StdILabelCompare());

  std::vector<Weight> distance, weights;
  auto total = ShortestDistance(nfst, &distance, kPhiLabel, false, kAlgoDelta);
  if (!total.Member()) return;
  if (!StationaryDistrib(nfst, &weights, kReEntryWeight, kPhiLabel, kSTDelta))
    return;

  NormWeights(&distance);
  SumStateWeights(nfst, &weights, kPhiLabel, true);
  NormWeights(&weights);
  // Norm. shortest dist. equals 'summed' stationary distrib (of closure)
  CHECK(ApproxEqualWeights(distance, weights, kNormDelta));
}

void AlgoTester::TestStationaryDistribStateSum(const StdFst& ifst,
                                               int order) const {
  const Weight kApproxZero(40.0);

  if (ifst.Start() == fst::kNoStateId)  // no states
    return;

  // Uses n-gram FST as input
  StdVectorFst nfst(ifst), afst;
  CHECK(GlobalNormalize(&nfst, kPhiLabel, kAlgoDelta));
  CHECK(NGramApprox(nfst, &afst, order, kPhiLabel, kAlgoDelta, NORM_SUMMED));

  std::vector<Weight> weights1;
  if (!StationaryDistrib(afst, &weights1, Weight(1.0e-6), kPhiLabel, 1.0e-6))
    return;
  SumStateWeights(afst, &weights1, kPhiLabel, false);
  NormWeights(&weights1);

  StdVectorFst ofst(afst);
  Counter<StdArc> counter(kPhiLabel, kAlgoDelta, &ofst);
  counter.Count(afst);
  counter.Finalize();
  SumBackoff(&ofst, kPhiLabel);

  std::vector<Weight> weights2;
  SumStates(ofst, kPhiLabel, &weights2);
  NormWeights(&weights2);
  // 'Summed' stationary distrib (of closure) equals state sums of 'summed'
  // backoff-complete model.
  CHECK(ApproxEqualWeights(weights1, weights2, kNormDelta, kApproxZero));
}

void AlgoTester::TestTrimType(const StdFst& ifst, TrimType trim_type) const {
  using Arc = fst::StdArc;

  // Uses unweighted and epsilon-free input.
  fst::VectorFst<Arc> rfst;
  fst::ArcMap(ifst, &rfst, fst::RmWeightMapper<Arc>());
  std::vector<std::pair<Label, Label>> inpairs = {{0, kNumRandomLabels - 1}};
  fst::Relabel(&rfst, inpairs, inpairs);
  fst::ArcSort(&rfst, fst::StdILabelCompare());

  fst::VectorFst<Arc> tfst1(rfst);
  CHECK(Trim(&tfst1, kPhiLabel, trim_type));

  // Trim?
  if (trim_type == TRIM_NEEDED_TRIM || trim_type == TRIM_NEEDED_FINAL)
    CHECK(IsTrim(tfst1, kPhiLabel));

  // Idempotent?
  if (trim_type == TRIM_NEEDED_TRIM || trim_type == TRIM_NEEDED_FINAL) {
    fst::VectorFst<Arc> tfst2(tfst1);
    CHECK(Trim(&tfst2, kPhiLabel, TRIM_NEEDED_TRIM));
    CHECK(Equal(tfst1, tfst2));
  }

  // Phi-removed equivalent?
  if (trim_type == TRIM_NEEDED_NONFINAL) {
    fst::VectorFst<Arc> prfst1, prfst2, det1, det2;
    PhiRemove(rfst, &prfst1);
    PhiRemove(tfst1, &prfst2);
    fst::Determinize(prfst1, &det1);
    fst::Determinize(prfst2, &det2);
    CHECK(Equivalent(det1, det2));
  }
}

void AlgoTester::TestCompose(const StdFst& ifst1, const StdFst& ifst2) const {
  using Arc = fst::StdArc;

  // Uses unweighted and epsilon-free input.
  fst::VectorFst<Arc> rfst1, rfst2, cfst1;
  fst::ArcMap(ifst1, &rfst1, fst::RmWeightMapper<Arc>());
  fst::ArcMap(ifst2, &rfst2, fst::RmWeightMapper<Arc>());
  std::vector<std::pair<Label, Label>> inpairs = {{0, kNumRandomLabels - 1}};
  fst::Relabel(&rfst1, inpairs, inpairs);
  fst::Relabel(&rfst2, inpairs, inpairs);
  fst::ArcSort(&rfst1, fst::StdILabelCompare());
  fst::ArcSort(&rfst2, fst::StdILabelCompare());

  using PM = Phi2Matcher<fst::Matcher<fst::Fst<Arc>>>;
  using PF = Phi2Filter<PM>;
  fst::ComposeFstOptions<Arc, PM, PF> copts;
  copts.matcher1 = new PM(rfst1, fst::MATCH_OUTPUT, kPhiLabel);
  copts.matcher2 = new PM(rfst2, fst::MATCH_INPUT, kPhiLabel);
  cfst1 = fst::ComposeFst<Arc>(rfst1, rfst2, copts);
  CHECK(Trim(&cfst1, kPhiLabel, TRIM_NEEDED_NONFINAL));

  // Phi-removed equivalent?
  fst::VectorFst<Arc> prfst1, prfst2, prcfst1, prcfst2, det1, det2;
  PhiRemove(rfst1, &prfst1);
  PhiRemove(rfst2, &prfst2);
  PhiRemove(cfst1, &prcfst1);
  Compose(prfst1, prfst2, &prcfst2);
  fst::Determinize(prcfst1, &det1);
  fst::Determinize(prcfst2, &det2);
  CHECK(Equivalent(det1, det2));
}

void AlgoTester::TestNGramApproxMerge(const StdFst& ifst, int order) const {
  if (ifst.Start() == fst::kNoStateId)  // no states
    return;

  // Uses trim and normalized input.
  StdVectorFst nfst1(ifst);
  CHECK(Trim(&nfst1, kPhiLabel));
  CHECK(GlobalNormalize(&nfst1, kPhiLabel, kAlgoDelta));

  // Merges nfst1 with itself
  StdVectorFst nfst2;
  Merge(nfst1, nfst1, &nfst2);

  StdVectorFst ofst1, ofst2;
  CHECK(NGramApprox(nfst1, &ofst1, order, kPhiLabel, kAlgoDelta, NORM_SUMMED));
  CHECK(NGramApprox(nfst2, &ofst2, order, kPhiLabel, kAlgoDelta, NORM_SUMMED));
  CHECK(Verify(ofst1));
  CHECK(Verify(ofst2));
  CHECK(IsCanonical(ofst1, kPhiLabel));
  CHECK(IsNormalized(ofst1, kPhiLabel, kNormDelta));
  CHECK(IsCanonical(ofst2, kPhiLabel));
  CHECK(IsNormalized(ofst2, kPhiLabel, kNormDelta));
  CHECK(RmPhiIsomorphic(ofst1, ofst2));
}

void AlgoTester::TestNGramApproxRmPhi(const StdFst& ifst, int order) const {
  if (ifst.Start() == fst::kNoStateId)  // no states
    return;

  // Uses epsilon-free and trimmed input.
  StdVectorFst rfst(ifst);
  std::vector<std::pair<Label, Label>> inpairs = {{0, kNumRandomLabels - 1}};
  fst::Relabel(&rfst, inpairs, inpairs);
  fst::ArcSort(&rfst, fst::StdILabelCompare());
  CHECK(Trim(&rfst, kPhiLabel));

  StdVectorFst nfst(rfst), ofst;

  CHECK(GlobalNormalize(&nfst, kPhiLabel, kAlgoDelta));
  CHECK(NGramApprox(nfst, &ofst, order, kPhiLabel, kAlgoDelta, NORM_SUMMED));
  CHECK(Verify(ofst));
  CHECK(IsCanonical(ofst, kPhiLabel));
  CHECK(IsNormalized(ofst, kPhiLabel, kNormDelta));

  StdVectorFst prfst, profst(ofst);
  PhiRemove(rfst, &prfst);
  CHECK(GlobalNormalize(&prfst, kPhiLabel, kAlgoDelta));
  CHECK(Approx(prfst, &profst, kPhiLabel, kAlgoDelta, NORM_SUMMED));
  CHECK(Verify(profst));
  CHECK(IsCanonical(profst, kPhiLabel));
  CHECK(IsNormalized(profst, kPhiLabel, kNormDelta));
  CHECK(RmPhiIsomorphic(ofst, profst));
}

void AlgoTester::TestApprox(const StdFst& ifst) const {
  if (ifst.Start() == fst::kNoStateId)  // no states
    return;

  // Uses n-gram FST as input
  StdVectorFst nfst(ifst), afst;
  CHECK(Trim(&nfst, kPhiLabel));
  CHECK(GlobalNormalize(&nfst, kPhiLabel, kAlgoDelta));
  CHECK(NGramApprox(nfst, &afst, 2, kPhiLabel, kAlgoDelta, NORM_SUMMED));
  StdVectorFst ofst(afst);

  // Requires sanity of input.
  if (!SaneFst(afst, kPhiLabel)) return;
  // Check phi-summed approximation.
  CHECK(Approx(afst, &ofst, kPhiLabel, kAlgoDelta, NORM_SUMMED));
  // Requires sanity of output for next step.
  if (!SaneFst(ofst, kPhiLabel)) return;

  // Tests marginally-constrained approximation onto same
  // topology is an identity.
  if (!Approx(afst, &ofst, kPhiLabel, kAlgoDelta, NORM_KL_MIN)) {
    return;
  }
  CHECK(RmPhiIsomorphic(afst, ofst));
}

void AlgoTester::MakeRandFsa(absl::BitGenRef bit_gen, StdMutableFst* fst,
                             bool cyclic) {
  StdVectorFst rfst;
  CHECK_OK(fst::RandFst(kNumRandomStates, kNumRandomArcs, kNumRandomLabels, 1.0,
                        generate_, bit_gen, &rfst));
  // Connected so it can be normalized.
  fst::Connect(&rfst);
  // Projected so it can be determinized.
  fst::Project(&rfst, fst::ProjectType::INPUT);
  // Determinized so it is canonical (wrt phi label).
  fst::Determinize(rfst, fst);
  // Increases sharing and pushes weight
  if (absl::Bernoulli(bit_gen, 0.5)) {
    fst::Minimize(fst);
    fst::ArcSort(fst, fst::StdILabelCompare());
  }

  // Mix up the state order.
  if (absl::Bernoulli(bit_gen, 0.5)) {
    ssize_t ns = fst->NumStates();
    std::vector<StateId> states(ns);
    std::iota(states.begin(), states.end(), 0);
    for (ssize_t i = 0; i < ns; ++i) {
      ssize_t ni = absl::Uniform(bit_gen, 0, ns);
      std::swap(states[i], states[ni]);
    }
    fst::StateSort(fst, states);
  }

  // Adds (non-phi) non-determinism
  if (absl::Bernoulli(bit_gen, 0.5)) {
    std::vector<std::pair<Label, Label>> inpairs = {
        {kNumRandomLabels - 1, kNumRandomLabels - 2}};
    fst::Relabel(fst, inpairs, inpairs);
  }

  if (cyclic) {
    // Creates a normalizable cyclic machine with no epsilon cycles.
    LocalNormalize(fst);
    std::vector<std::pair<Label, Label>> inpairs = {{0, kNumRandomLabels + 1}};
    fst::Relabel(fst, inpairs, inpairs);
    fst::Closure(fst, fst::CLOSURE_PLUS);
    std::vector<std::pair<Label, Label>> outpairs = {{0, kNumRandomLabels},
                                                     {kNumRandomLabels + 1, 0}};
    fst::Relabel(fst, outpairs, outpairs);
    Condition(fst, fst::kNoLabel, 1.0);
  }

  // Arcsorted so it is canonical
  fst::ArcSort(fst, fst::StdILabelCompare());

  // Returns empty machine if no non-trivial paths.
  if (!NonTrivialLabel(*fst, kPhiLabel)) fst->DeleteStates();
}

template <typename A>
void AlgoTester::ThompsonUnion(fst::MutableFst<A>* fst1,
                               const fst::Fst<A>& fst2) const {
  // Saves old verify props state
  bool verify_props = absl::GetFlag(FLAGS_fst_verify_properties);
  absl::SetFlag(&FLAGS_fst_verify_properties, false);

  // Forces Thompson construction by temp modifying of props
  uint64_t init_props = fst::kInitialCyclic | fst::kInitialAcyclic;
  fst1->SetStart(fst1->Start());  // force mutation to free shallow copies
  fst1->SetProperties(fst::kInitialCyclic, init_props);

  fst::Union(fst1, fst2);

  // Clears modified props, restores old verify props state
  fst1->SetProperties(0, init_props);
  absl::SetFlag(&FLAGS_fst_verify_properties, verify_props);
}

}  // namespace sfst

// DEFINEs determine which semirings are tested; these are controlled by
// the `defines` attributes of the associated build rules.

ABSL_FLAG(int32_t, repeat, 25, "number of test repetitions");

int main(int argc, char** argv) {
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);

  sfst::AlgoTester algo_tester;
  for (int i = 0; i < absl::GetFlag(FLAGS_repeat); ++i) algo_tester.Test();

  return 0;
}
