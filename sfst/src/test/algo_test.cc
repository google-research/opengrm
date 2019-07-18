
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
// Copyright 2018 Google, Inc.
// See www.opengrm.org for documentation on this stochastic weighted
// finite-state transducer library.
//
// Regression test for various SFST algorithms.

#include <stdlib.h>
#include <sys/types.h>

#include <iostream>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/fstlib.h>
#include <fst/test/rand-fst.h>
#include <sfst/approx.h>
#include <sfst/backoff.h>
#include <sfst/canonical.h>
#include <sfst/count.h>
#include <sfst/equal.h>
#include <sfst/ngramapprox.h>
#include <sfst/normalize.h>
#include <sfst/phi2matcher.h>
#include <sfst/shortest-distance.h>
#include <sfst/state-weights.h>
#include <sfst/stationary-distrib.h>
#include <sfst/trim.h>

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

  AlgoTester() : weight_generator_(false) {
    univ_fst_.AddState();
    univ_fst_.SetStart(0);
    univ_fst_.SetFinal(0, Weight::One());
    for (int i = 1; i <= kNumRandomLabels; ++i) {
      if (i != kPhiLabel) univ_fst_.AddArc(0, StdArc(i, i, Weight::One(), 0));
    }
  }

  void Test() const {
    StdVectorFst fst1, fst2;
    bool cyclic1 = rand() % 2;  // NOLINT
    bool cyclic2 = rand() % 2;  // NOLINT
    MakeRandFsa(&fst1, cyclic1);
    MakeRandFsa(&fst2, cyclic2);
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
  void TestCanonical(const StdFst &ifst) const {
    CHECK(IsCanonical(ifst, kPhiLabel));
  }

  // Checks phi shortest distance algos.
  void TestShortestDistance(const StdFst &ifst) const;

  void TestStationaryDistrib(const StdFst &ifst) const {
    TestStationaryDistribShDist(ifst);
    TestStationaryDistribStateSum(ifst, 2);
    TestStationaryDistribStateSum(ifst, 3);
  }

  // Checks stationary distribution and its relation to
  // shortest distance.
  void TestStationaryDistribShDist(const StdFst &ifst) const;

  // Checks stationary distribution and its relation to state sums.
  void TestStationaryDistribStateSum(const StdFst &ifst, int order) const;

  // Checks trimming is idempotent and equivalent (after phi-removal)
  // appropriate.
  void TestTrim(const StdFst &ifst) const {
    TestTrimType(ifst, TRIM_NEEDED_TRIM);
    TestTrimType(ifst, TRIM_NEEDED_NONFINAL);
    TestTrimType(ifst, TRIM_NEEDED_FINAL);
  }

  // Checks trimming is idempotent and equivalent (after phi-removal)
  // depending on trim type.
  void TestTrimType(const StdFst &ifst, TrimType trim_type) const;

  // Checks composition commutes with phi_removal.
  void TestCompose(const StdFst &ifst1, const StdFst &ifst2) const;

  // Checks input can be (globally) normalized
  void TestNormalize(const StdFst &ifst) const {
    StdVectorFst ofst(ifst);
    CHECK(GlobalNormalize(&ofst, kPhiLabel, kAlgoDelta));
    CHECK(IsNormalized(ofst, kPhiLabel, kNormDelta));
  }

  // Checks ngram-approximation
  void TestNGramApprox(const StdFst &fst) const {
    TestNGramApproxNorm(fst, 2);
    TestNGramApproxNorm(fst, 3);
    TestNGramApproxMerge(fst, 2);
    TestNGramApproxMerge(fst, 3);
    TestNGramApproxRmPhi(fst, 2);
    TestNGramApproxRmPhi(fst, 3);
  }

  // Checks input can be ngram-approximated and result is normalized
  void TestNGramApproxNorm(const StdFst &ifst, int order) const {
    namespace f = fst;
    if (ifst.Start() == f::kNoStateId)  // no states
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
  void TestNGramApproxMerge(const StdFst &ifst, int order) const;

  // Checks input ngram-approximation is unaffected
  // by input FST phi-removal.
  void TestNGramApproxRmPhi(const StdFst &ifst, int order) const;


  // Test SFST approximation.
  void TestApprox(const StdFst &ifst) const {
    namespace f = fst;
    if (ifst.Start() == f::kNoStateId)  // no states
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

 private:
  // Generates a canonical but unnormalized stochastic FSA
  void MakeRandFsa(StdMutableFst *fst, bool cyclic) const;

  // Checks there is a non-trivial label.
  bool NonTrivialLabel(const StdExpandedFst &fst,
                       StdArc::Label phi_label) const {
    namespace f = fst;
    for (StateId s = 0; s < fst.NumStates(); ++s) {
      for (f::ArcIterator<StdExpandedFst> aiter(fst, s); !aiter.Done();
           aiter.Next()) {
        const auto &arc = aiter.Value();
        if (arc.ilabel && arc.ilabel != phi_label) return true;
      }
    }
    return false;
  }

  // fst::Union may change the semantics of failure transitions
  // but the Thompson construction won't (which is forced if
  // fst::Union believes there is an initial cycle on fst1).
  template <typename A>
  void ThompsonUnion(fst::MutableFst<A> *fst1,
                     const fst::Fst<A> &fst2) const;

  // (Non-det) unions two FSTs and normalizes (assuming kPhiLabel).
  void Merge(const StdFst &fst1, const StdFst &fst2,
             StdMutableFst *ofst) const {
    namespace f = fst;
    *ofst = fst1;
    ThompsonUnion(ofst, fst1);
    f::ArcSort(ofst, f::StdILabelCompare());
    CHECK(GlobalNormalize(ofst, kPhiLabel, kAlgoDelta));
  }

  void PhiRemove(const StdFst &ifst, StdMutableFst *ofst) const {
    namespace f = fst;

    using PM = Phi2Matcher<f::Matcher<f::StdFst>>;
    using PF = Phi2Filter<PM>;
    f::ComposeFstOptions<StdArc, PM, PF> copts;

    copts.gc_limit = 0;
    copts.matcher1 = new PM(ifst, f::MATCH_OUTPUT, kPhiLabel);
    copts.matcher2 = new PM(univ_fst_, f::MATCH_NONE, f::kNoLabel);
    *ofst = StdComposeFst(ifst, univ_fst_, copts);
    f::Connect(ofst);
  }

  // Tests isomorphic after phi-removal
  bool RmPhiIsomorphic(const StdFst &fst1, const StdFst &fst2) const {
    StdVectorFst rfst1, rfst2;
    PhiRemove(fst1, &rfst1);
    PhiRemove(fst1, &rfst2);
    return Isomorphic(rfst1, rfst2, kPhiLabel, kNormDelta);
  }

  // Ensures trim and sane arc weights.
  bool SaneFst(const StdExpandedFst &fst, Label phi_label) const {
    namespace f = fst;
    if (!IsTrim(fst, phi_label))
      return false;

    // Minimum weight allowed for an arc.
    static const f::Log64Weight kWeightThreshold = 10.0;

    f::WeightConvert<Weight, f::Log64Weight> to_log;
    for (StateId s = 0; s < fst.NumStates(); ++s) {
      for (f::ArcIterator<StdExpandedFst> aiter(fst, s); !aiter.Done();
           aiter.Next()) {
        const auto &arc = aiter.Value();
        if (Less(to_log(arc.weight), kWeightThreshold))
          return false;
      }
    }
    return true;
  }


  mutable WeightGenerator weight_generator_;
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

void AlgoTester::TestShortestDistance(const StdFst &ifst) const {
  namespace f = fst;
  using Arc = f::StdArc;
  using Weight = Arc::Weight;
  using LArc = f::Log64Arc;
  using LWeight = LArc::Weight;
  using WCM = f::WeightConvertMapper<Arc, LArc>;
  using SFM = f::SuperFinalMapper<LArc>;

  f::VectorFst<LArc> lfst;
  WCM wc_mapper;
  SFM sf_mapper;
  f::ArcMap(ifst, &lfst, wc_mapper);
  f::ArcMap(&lfst, sf_mapper);

  f::VectorFst<LArc> cfst(lfst), ufst(lfst);
  f::Concat(&cfst, lfst);
  ThompsonUnion(&ufst, lfst);

  std::vector<LWeight> distance, rdistance, cdistance, udistance;
  PhiShortestDistance(lfst, &distance, kPhiLabel, false, kAlgoDelta);
  PhiShortestDistance(lfst, &rdistance, kPhiLabel, true, kAlgoDelta);
  PhiShortestDistance(cfst, &cdistance, kPhiLabel, false, kAlgoDelta);
  PhiShortestDistance(ufst, &udistance, kPhiLabel, false, kAlgoDelta);

  // This computation assumes all super-final arcs can be
  // read. Using a superfinal state as above ensures this is
  // correct.
  LWeight fw = f::ComputeTotalWeight(lfst, distance, false);
  LWeight rw = f::ComputeTotalWeight(lfst, rdistance, true);
  LWeight cw = f::ComputeTotalWeight(cfst, cdistance, false);
  LWeight uw = f::ComputeTotalWeight(ufst, udistance, false);

  CHECK(ApproxEqual(fw, rw, kNormDelta));
  CHECK(ApproxEqual(cw, Times(fw, fw), kNormDelta));
  CHECK(ApproxEqual(uw, Plus(fw, fw), kNormDelta));
}

void AlgoTester::TestStationaryDistribShDist(const StdFst &ifst) const {
  namespace f = fst;
  const Weight kReEntryWeight = 1.0e-6;
  const float kSTDelta = 1.0e-6;

  if (ifst.Start() == f::kNoStateId)  // no states
    return;

  // Uses trim, normalized and eps-free input with a superfinal state.
  StdVectorFst nfst;
  f::ArcMap(ifst, &nfst, f::SuperFinalMapper<StdArc>());
  f::ArcSort(&nfst, f::StdILabelCompare());

  CHECK(Trim(&nfst, kPhiLabel));
  CHECK(GlobalNormalize(&nfst, kPhiLabel, kAlgoDelta));
  std::vector<std::pair<Label, Label>> inpairs = {{0, kNumRandomLabels - 1}};
  f::Relabel(&nfst, inpairs, inpairs);
  f::ArcSort(&nfst, f::StdILabelCompare());

  std::vector<Weight> distance, weights;
  if (!PhiShortestDistance(nfst, &distance, kPhiLabel, false, kAlgoDelta))
    return;
  if (!PhiStationaryDistrib(nfst, &weights, kReEntryWeight, kPhiLabel,
                            kSTDelta))
    return;

  NormWeights(&distance);
  SumStateWeights(nfst, &weights, kPhiLabel, true);
  NormWeights(&weights);
  // Norm. shortest dist. equals 'summed' stationary distrib (of closure)
  CHECK(ApproxEqualWeights(distance, weights, kNormDelta));
}

void AlgoTester::TestStationaryDistribStateSum(const StdFst &ifst,
                                               int order) const {
  namespace f = fst;
  const Weight kApproxZero = 40.0;

  if (ifst.Start() == f::kNoStateId)  // no states
    return;

  // Uses n-gram FST as input
  StdVectorFst nfst(ifst), afst;
  CHECK(GlobalNormalize(&nfst, kPhiLabel, kAlgoDelta));
  CHECK(NGramApprox(nfst, &afst, order, kPhiLabel, kAlgoDelta, NORM_SUMMED));

  std::vector<Weight> weights1;
  if (!PhiStationaryDistrib(afst, &weights1, 1.0e-6, kPhiLabel, 1.0e-6))
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

void AlgoTester::TestTrimType(const StdFst &ifst, TrimType trim_type) const {
  namespace f = fst;
  using Arc = f::StdArc;

  // Uses unweighted and epsilon-free input.
  f::VectorFst<Arc> rfst;
  f::ArcMap(ifst, &rfst, f::RmWeightMapper<Arc>());
  std::vector<std::pair<Label, Label>> inpairs = {{0, kNumRandomLabels - 1}};
  f::Relabel(&rfst, inpairs, inpairs);
  f::ArcSort(&rfst, f::StdILabelCompare());

  f::VectorFst<Arc> tfst1(rfst);
  CHECK(Trim(&tfst1, kPhiLabel, trim_type));

  // Trim?
  if (trim_type == TRIM_NEEDED_TRIM || trim_type == TRIM_NEEDED_FINAL)
    CHECK(IsTrim(tfst1, kPhiLabel));

  // Idempotent?
  if (trim_type == TRIM_NEEDED_TRIM || trim_type == TRIM_NEEDED_FINAL) {
    f::VectorFst<Arc> tfst2(tfst1);
    CHECK(Trim(&tfst2, kPhiLabel, TRIM_NEEDED_TRIM));
    CHECK(Equal(tfst1, tfst2));
  }

  // Phi-removed equivalent?
  if (trim_type == TRIM_NEEDED_NONFINAL) {
    f::VectorFst<Arc> prfst1, prfst2, det1, det2;
    PhiRemove(rfst, &prfst1);
    PhiRemove(tfst1, &prfst2);
    f::Determinize(prfst1, &det1);
    f::Determinize(prfst2, &det2);
    CHECK(Equivalent(det1, det2));
  }
}

void AlgoTester::TestCompose(const StdFst &ifst1, const StdFst &ifst2) const {
  namespace f = fst;
  using Arc = f::StdArc;

  // Uses unweighted and epsilon-free input.
  f::VectorFst<Arc> rfst1, rfst2, cfst1;
  f::ArcMap(ifst1, &rfst1, f::RmWeightMapper<Arc>());
  f::ArcMap(ifst2, &rfst2, f::RmWeightMapper<Arc>());
  std::vector<std::pair<Label, Label>> inpairs = {{0, kNumRandomLabels - 1}};
  f::Relabel(&rfst1, inpairs, inpairs);
  f::Relabel(&rfst2, inpairs, inpairs);
  f::ArcSort(&rfst1, f::StdILabelCompare());
  f::ArcSort(&rfst2, f::StdILabelCompare());

  using PM = Phi2Matcher<f::Matcher<f::Fst<Arc>>>;
  using PF = Phi2Filter<PM>;
  f::ComposeFstOptions<Arc, PM, PF> copts;
  copts.matcher1 = new PM(rfst1, f::MATCH_OUTPUT, kPhiLabel);
  copts.matcher2 = new PM(rfst2, f::MATCH_INPUT, kPhiLabel);
  cfst1 = f::ComposeFst<Arc>(rfst1, rfst2, copts);
  CHECK(Trim(&cfst1, kPhiLabel, TRIM_NEEDED_NONFINAL));

  // Phi-removed equivalent?
  f::VectorFst<Arc> prfst1, prfst2, prcfst1, prcfst2, det1, det2;
  PhiRemove(rfst1, &prfst1);
  PhiRemove(rfst2, &prfst2);
  PhiRemove(cfst1, &prcfst1);
  Compose(prfst1, prfst2, &prcfst2);
  f::Determinize(prcfst1, &det1);
  f::Determinize(prcfst2, &det2);
  CHECK(Equivalent(det1, det2));
}

void AlgoTester::TestNGramApproxMerge(const StdFst &ifst, int order) const {
  namespace f = fst;
  if (ifst.Start() == f::kNoStateId)  // no states
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

void AlgoTester::TestNGramApproxRmPhi(const StdFst &ifst, int order) const {
  namespace f = fst;
  if (ifst.Start() == f::kNoStateId)  // no states
    return;

  // Uses epsilon-free and trimmed input.
  StdVectorFst rfst(ifst);
  std::vector<std::pair<Label, Label>> inpairs = {{0, kNumRandomLabels - 1}};
  f::Relabel(&rfst, inpairs, inpairs);
  f::ArcSort(&rfst, f::StdILabelCompare());
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

void AlgoTester::MakeRandFsa(StdMutableFst *fst,
                             bool cyclic) const {
  StdVectorFst rfst;
  namespace f = fst;
  f::RandFst<StdArc, WeightGenerator>(kNumRandomStates, kNumRandomArcs,
                                      kNumRandomLabels, 1.0,
                                      &weight_generator_, &rfst);
  // Connected so it can be normalized.
  f::Connect(&rfst);
  // Projected so it can be determinized.
  f::Project(&rfst, f::PROJECT_INPUT);
  // Determinized so it is canonical (wrt phi label).
  f::Determinize(rfst, fst);
  // Increases sharing and pushes weight
  if (rand() % 2) {  // NOLINT
    f::Minimize(fst);
    f::ArcSort(fst, f::StdILabelCompare());
  }

  // Mixs up the state order
  if (rand() % 2) {  // NOLINT
    ssize_t ns = fst->NumStates();
    std::vector<StateId> states(ns);
    std::iota(states.begin(), states.end(), 0);
    for (ssize_t i = 0; i < ns; ++i) {
      ssize_t ni = rand() % ns;  // NOLINT
      std::swap(states[i], states[ni]);
    }
    f::StateSort(fst, states);
  }

  // Adds (non-phi) non-determinism
  if (rand() % 2) {  // NOLINT
    std::vector<std::pair<Label, Label>> inpairs = {
        {kNumRandomLabels - 1, kNumRandomLabels - 2}};
    f::Relabel(fst, inpairs, inpairs);
  }

  if (cyclic) {
    // Creates a normalizable cyclic machine with no epsilon cycles.
    LocalNormalize(fst);
    std::vector<std::pair<Label, Label>> inpairs = {
        {0, kNumRandomLabels + 1}};
    f::Relabel(fst, inpairs, inpairs);
    f::Closure(fst, f::CLOSURE_PLUS);
    std::vector<std::pair<Label, Label>> outpairs = {
        {0, kNumRandomLabels}, {kNumRandomLabels + 1, 0}};
    f::Relabel(fst, outpairs, outpairs);
    Condition(fst, f::kNoLabel, 1.0);
  }

  // Arcsorted so it is canonical
  f::ArcSort(fst, f::StdILabelCompare());

  // Returns empty machine if no non-trivial paths.
  if (!NonTrivialLabel(*fst, kPhiLabel)) fst->DeleteStates();
}

template <typename A>
void AlgoTester::ThompsonUnion(fst::MutableFst<A> *fst1,
                   const fst::Fst<A> &fst2) const {
  namespace f = fst;
  // Saves old verify props state
  bool verify_props = FLAGS_fst_verify_properties;
  FLAGS_fst_verify_properties = false;

  // Forces Thompson construction by temp modifying of props
  uint64 init_props = f::kInitialCyclic | f::kInitialAcyclic;
  fst1->SetStart(fst1->Start());  // force mutation to free shallow copies
  fst1->SetProperties(f::kInitialCyclic, init_props);

  f::Union(fst1, fst2);

  // Clears modified props, restores old verify props state
  fst1->SetProperties(0, init_props);
  FLAGS_fst_verify_properties = verify_props;
}


}  // namespace sfst

// DEFINEs determine which semirings are tested; these are controlled by
// the `defines` attributes of the associated build rules.

DEFINE_int32(seed, -1, "random seed");
DEFINE_int32(repeat, 25, "number of test repetitions");

int main(int argc, char **argv) {
  FLAGS_fst_verify_properties = true;
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(argv[0], &argc, &argv, true);

  srand(FLAGS_seed);
  LOG(INFO) << "Seed = " << FLAGS_seed;

  sfst::AlgoTester algo_tester;
  for (int i = 0; i < FLAGS_repeat; ++i) algo_tester.Test();

  std::cout << "PASS" << std::endl;

  return 0;
}
