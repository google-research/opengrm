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

// Unit tests for Baum-Welch stepwise EM and Viterbi training.

#include "opengrm/baumwelch/train.h"

#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/strings/string_view.h"
#include "openfst/extensions/far/far-reader.h"
#include "openfst/extensions/far/far-type.h"
#include "openfst/extensions/far/far-writer.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/string.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/weight.h"
#include "opengrm/baumwelch/cascade.h"
#include "opengrm/baumwelch/expectation-table.h"
#include "opengrm/baumwelch/log-adder.h"

namespace fst {
namespace {

// -----------------------------------------------------------------------------
// TrainOptions Tests
// -----------------------------------------------------------------------------

TEST(TrainOptionsTest, DefaultValues) {
  const TrainOptions opts;
  EXPECT_EQ(opts.max_iters, kMaxIters);
  EXPECT_FLOAT_EQ(opts.alpha, kAlpha);
  EXPECT_EQ(opts.batch_size, 0);
  EXPECT_FLOAT_EQ(opts.delta, kDelta);
}

TEST(TrainOptionsTest, AlphaZeroForcesFullBatch) {
  const TrainOptions opts(/*max_iters=*/20, /*alpha=*/0.0, /*batch_size=*/10);
  EXPECT_EQ(opts.max_iters, 20);
  EXPECT_FLOAT_EQ(opts.alpha, 0.0);
  EXPECT_EQ(opts.batch_size, 0);  // Must be 0 when alpha is 0.
}

TEST(TrainOptionsTest, CustomValues) {
  const CascadeOptions copts;
  const TrainOptions opts(/*max_iters=*/15, /*alpha=*/0.5, /*batch_size=*/4,
                          /*delta=*/1e-4, copts);
  EXPECT_EQ(opts.max_iters, 15);
  EXPECT_FLOAT_EQ(opts.alpha, 0.5);
  EXPECT_EQ(opts.batch_size, 4);
  EXPECT_FLOAT_EQ(opts.delta, 1e-4);
}

// -----------------------------------------------------------------------------
// ForwardBackward Tests
// -----------------------------------------------------------------------------

TEST(ForwardBackwardTest, ComputesAlphaBetaOnLattice) {
  // Lattice with two parallel paths:
  // s0 --(1:1, w=1.0)--> s1 (w_final=0.5)
  // s0 --(2:2, w=2.0)--> s2 (w_final=1.5)
  VectorFst<LogArc> model;
  const auto s0 = model.AddState();
  model.SetStart(s0);
  const auto s1 = model.AddState();
  const auto s2 = model.AddState();

  model.AddArc(s0, LogArc(1, 1, LogWeight(1.0), s1));
  model.AddArc(s0, LogArc(2, 2, LogWeight(2.0), s2));
  model.SetFinal(s1, LogWeight(0.5));
  model.SetFinal(s2, LogWeight(1.5));

  // Input accepts 1 or 2
  VectorFst<LogArc> input;
  const auto i0 = input.AddState();
  input.SetStart(i0);
  const auto i1 = input.AddState();
  input.AddArc(i0, LogArc(1, 1, LogWeight::One(), i1));
  input.AddArc(i0, LogArc(2, 2, LogWeight::One(), i1));
  input.SetFinal(i1, LogWeight::One());

  VectorFst<LogArc> output = input;

  const SimpleCascade<LogArc> cascade(input, output, model);
  const internal::ForwardBackward<LogArc> fb(cascade.GetFst());

  const auto start = cascade.GetFst().Start();
  ASSERT_NE(start, kNoStateId);

  // Alpha at start should be LogWeight::One() (0.0).
  EXPECT_EQ(fb.Alpha(start), LogWeight::One());

  // Beta at start is the total lattice likelihood:
  // LogAdder(1.0+0.5, 2.0+1.5) = LogAdder(1.5, 3.5).
  LogAdder<LogWeight> expected_total;
  expected_total.Add(LogWeight(1.5));
  expected_total.Add(LogWeight(3.5));
  EXPECT_NEAR(fb.Beta(start).Value(), expected_total.Sum().Value(), 1e-4);

  // Unvisited or out of bounds state should return Zero().
  EXPECT_EQ(fb.Alpha(9999), LogWeight::Zero());
  EXPECT_EQ(fb.Beta(9999), LogWeight::Zero());
  EXPECT_EQ(fb.Alpha(kNoStateId), LogWeight::Zero());
  EXPECT_EQ(fb.Beta(kNoStateId), LogWeight::Zero());
  EXPECT_FALSE(fb.Error());
}

TEST(ForwardBackwardTest, ReportsShortestDistanceError) {
  VectorFst<LogArc> model;
  const auto s0 = model.AddState();
  const auto s1 = model.AddState();
  model.SetStart(s0);
  model.AddArc(s0, LogArc(1, 1, LogWeight::NoWeight(), s1));
  model.SetFinal(s1, LogWeight::One());

  VectorFst<LogArc> str;
  const auto i0 = str.AddState();
  const auto i1 = str.AddState();
  str.SetStart(i0);
  str.AddArc(i0, LogArc(1, 1, LogWeight::One(), i1));
  str.SetFinal(i1, LogWeight::One());

  const SimpleCascade<LogArc> cascade(str, str, model);
  const internal::ForwardBackward<LogArc> fb(cascade.GetFst());
  EXPECT_TRUE(fb.Error());
  EXPECT_EQ(fb.Beta(cascade.GetFst().Start()), LogWeight::Zero());
  EXPECT_EQ(fb.Alpha(cascade.GetFst().Start()), LogWeight::Zero());
}

// -----------------------------------------------------------------------------
// Expectation Normalization Tests
// -----------------------------------------------------------------------------

using ArcTypes = ::testing::Types<StdArc, LogArc, Log64Arc>;

template <typename Arc>
class TrainNormalizeTest : public ::testing::Test {};

TYPED_TEST_SUITE(TrainNormalizeTest, ArcTypes, );

TYPED_TEST(TrainNormalizeTest, NormalizesStateExpectationTable) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;

  VectorFst<Arc> model;
  const auto s0 = model.AddState();
  const auto s1 = model.AddState();
  model.SetStart(s0);

  model.AddArc(s0, Arc(1, 10, Weight(2.0), s1));
  model.AddArc(s0, Arc(2, 20, Weight(3.0), s1));
  model.SetFinal(s0, Weight(1.0));
  model.SetFinal(s1, Weight(0.5));

  internal::Trainer<Arc, StateExpectationTable<Arc>>::Normalize(&model);

  // For state 0, outgoing arcs + final weight should sum to Weight::One()
  // (prob 1.0).
  LogAdder<Weight> sum_s0;
  for (ArcIterator<VectorFst<Arc>> aiter(model, s0); !aiter.Done();
       aiter.Next()) {
    sum_s0.Add(aiter.Value().weight);
  }
  sum_s0.Add(model.Final(s0));
  EXPECT_NEAR(sum_s0.Sum().Value(), Weight::One().Value(), 1e-4);

  // For state 1, only final weight exists, so it should normalize to
  // Weight::One().
  EXPECT_NEAR(model.Final(s1).Value(), Weight::One().Value(), 1e-4);
}

TYPED_TEST(TrainNormalizeTest, NormalizesStateILabelExpectationTable) {
  using Arc = TypeParam;
  using Weight = typename Arc::Weight;

  VectorFst<Arc> model;
  const auto s0 = model.AddState();
  const auto s1 = model.AddState();
  const auto s2 = model.AddState();
  model.SetStart(s0);

  // Two arcs with ilabel=1
  model.AddArc(s0, Arc(1, 10, Weight(1.0), s1));
  model.AddArc(s0, Arc(1, 20, Weight(2.0), s2));
  // One arc with ilabel=2
  model.AddArc(s0, Arc(2, 30, Weight(3.0), s1));
  // Final weight (ilabel=kNoLabel)
  model.SetFinal(s0, Weight(4.0));

  internal::Trainer<Arc, StateILabelExpectationTable<Arc>>::Normalize(&model);

  // Arcs with ilabel=1 should sum to Weight::One()
  LogAdder<Weight> sum_ilabel1;
  for (ArcIterator<VectorFst<Arc>> aiter(model, s0); !aiter.Done();
       aiter.Next()) {
    if (aiter.Value().ilabel == 1) {
      sum_ilabel1.Add(aiter.Value().weight);
    }
  }
  EXPECT_NEAR(sum_ilabel1.Sum().Value(), Weight::One().Value(), 1e-4);

  // Arc with ilabel=2 is unique, so normalized weight should be Weight::One()
  for (ArcIterator<VectorFst<Arc>> aiter(model, s0); !aiter.Done();
       aiter.Next()) {
    if (aiter.Value().ilabel == 2) {
      EXPECT_NEAR(aiter.Value().weight.Value(), Weight::One().Value(), 1e-4);
    }
  }

  // Final weight is unique for s0, so should be Weight::One()
  EXPECT_NEAR(model.Final(s0).Value(), Weight::One().Value(), 1e-4);
}

// -----------------------------------------------------------------------------
// Baum-Welch and Viterbi Training Tests
// -----------------------------------------------------------------------------

class TrainExecutionTest : public ::testing::Test {
 protected:
  void SetUp() override {
    input_far_path_ =
        fst::JoinPath(::testing::TempDir(), "train_test_input.far");
    output_far_path_ =
        fst::JoinPath(::testing::TempDir(), "train_test_output.far");
  }

  template <class Arc>
  void WriteTrainingData(const std::vector<std::string>& inputs,
                         const std::vector<std::string>& outputs) {
    ASSERT_EQ(inputs.size(), outputs.size());
    std::unique_ptr<FarWriter<Arc>> in_writer(
        FarWriter<Arc>::Create(input_far_path_));
    std::unique_ptr<FarWriter<Arc>> out_writer(
        FarWriter<Arc>::Create(output_far_path_));
    ASSERT_NE(in_writer, nullptr);
    ASSERT_NE(out_writer, nullptr);

    const StringCompiler<Arc> compiler(TokenType::SYMBOL);
    for (size_t i = 0; i < inputs.size(); ++i) {
      const std::string key = std::to_string(i);
      VectorFst<Arc> in_fst, out_fst;
      ASSERT_TRUE(compiler(inputs[i], &in_fst));
      ASSERT_TRUE(compiler(outputs[i], &out_fst));
      in_writer->Add(key, in_fst);
      out_writer->Add(key, out_fst);
    }
  }

  std::string input_far_path_;
  std::string output_far_path_;
};

TEST_F(TrainExecutionTest, BaumWelchEMLogArcLearnsIdentityTransduction) {
  // Training data: input "1 2", output "1 2" (repeated 5 times)
  const std::vector<std::string> data = {"1 2", "1 2", "1 2", "2 1", "2 1"};
  WriteTrainingData<LogArc>(data, data);

  std::unique_ptr<FarReader<LogArc>> in_reader(
      FarReader<LogArc>::Open(input_far_path_));
  std::unique_ptr<FarReader<LogArc>> out_reader(
      FarReader<LogArc>::Open(output_far_path_));
  ASSERT_NE(in_reader, nullptr);
  ASSERT_NE(out_reader, nullptr);

  // Model: 1-state transducer with noisy initial weights for all (i:o) pairs
  // Alphabet: {1, 2}.
  VectorFst<LogArc> model;
  const auto s0 = model.AddState();
  model.SetStart(s0);

  // Initialize with perturbed weights where incorrect pairs have lower initial
  // cost to prove that EM corrects them towards the ground truth data.
  model.AddArc(s0, LogArc(1, 1, LogWeight(2.0), s0));  // correct
  model.AddArc(s0, LogArc(1, 2, LogWeight(0.5), s0));  // incorrect
  model.AddArc(s0, LogArc(2, 2, LogWeight(2.0), s0));  // correct
  model.AddArc(s0, LogArc(2, 1, LogWeight(0.5), s0));  // incorrect
  model.SetFinal(s0, LogWeight(0.5));

  TrainOptions opts;
  opts.max_iters = 20;
  opts.delta = 1e-4;

  const auto final_likelihood = Train<LogArc>(*in_reader, *out_reader, &model,
                                              /*normalize_ilabel=*/true, opts);

  EXPECT_NE(final_likelihood, LogWeight::Zero());

  // Extract trained arc weights
  double w_11 = 0, w_12 = 0, w_22 = 0, w_21 = 0;
  for (ArcIterator<VectorFst<LogArc>> aiter(model, s0); !aiter.Done();
       aiter.Next()) {
    const auto& arc = aiter.Value();
    if (arc.ilabel == 1 && arc.olabel == 1) w_11 = arc.weight.Value();
    if (arc.ilabel == 1 && arc.olabel == 2) w_12 = arc.weight.Value();
    if (arc.ilabel == 2 && arc.olabel == 2) w_22 = arc.weight.Value();
    if (arc.ilabel == 2 && arc.olabel == 1) w_21 = arc.weight.Value();
  }

  // After training on identity pairs, P(1:1) should be >> P(1:2),
  // which means -log(P(1:1)) < -log(P(1:2)).
  EXPECT_LT(w_11, w_12);
  EXPECT_LT(w_22, w_21);
}

TEST_F(TrainExecutionTest, ViterbiEMStdArcLearnsIdentityTransduction) {
  const std::vector<std::string> data = {"1 2", "1 2", "2 1", "2 1"};
  WriteTrainingData<StdArc>(data, data);

  std::unique_ptr<FarReader<StdArc>> in_reader(
      FarReader<StdArc>::Open(input_far_path_));
  std::unique_ptr<FarReader<StdArc>> out_reader(
      FarReader<StdArc>::Open(output_far_path_));
  ASSERT_NE(in_reader, nullptr);
  ASSERT_NE(out_reader, nullptr);

  VectorFst<StdArc> model;
  const auto s0 = model.AddState();
  model.SetStart(s0);

  // Equal initial weights
  model.AddArc(s0, StdArc(1, 1, TropicalWeight(1.0), s0));
  model.AddArc(s0, StdArc(1, 2, TropicalWeight(1.0), s0));
  model.AddArc(s0, StdArc(2, 2, TropicalWeight(1.0), s0));
  model.AddArc(s0, StdArc(2, 1, TropicalWeight(1.0), s0));
  model.SetFinal(s0, TropicalWeight(0.0));

  TrainOptions opts;
  opts.max_iters = 10;
  opts.delta = 1e-4;

  const auto final_likelihood = Train<StdArc>(*in_reader, *out_reader, &model,
                                              /*normalize_ilabel=*/true, opts);

  EXPECT_NE(final_likelihood, TropicalWeight::Zero());
}

TEST_F(TrainExecutionTest, HandlesEmptyLattice) {
  // Input "1", output "9" with model having only labels 1 and 2
  WriteTrainingData<LogArc>({"1"}, {"9"});

  std::unique_ptr<FarReader<LogArc>> in_reader(
      FarReader<LogArc>::Open(input_far_path_));
  std::unique_ptr<FarReader<LogArc>> out_reader(
      FarReader<LogArc>::Open(output_far_path_));
  ASSERT_NE(in_reader, nullptr);
  ASSERT_NE(out_reader, nullptr);

  VectorFst<LogArc> model;
  const auto s0 = model.AddState();
  model.SetStart(s0);
  model.AddArc(s0, LogArc(1, 1, LogWeight(0.5), s0));
  model.SetFinal(s0, LogWeight(0.0));

  TrainOptions opts;
  opts.max_iters = 2;

  // Composition is empty because model cannot produce output label 9.
  // Training should handle gracefully without crashing and return
  // LogWeight::Zero().
  const auto final_likelihood = Train<LogArc>(*in_reader, *out_reader, &model,
                                              /*normalize_ilabel=*/true, opts);
  EXPECT_EQ(final_likelihood, LogWeight::Zero());
}

TEST_F(TrainExecutionTest, MinibatchStepwiseTraining) {
  const std::vector<std::string> data = {"1", "2", "1", "2", "1", "2"};
  WriteTrainingData<LogArc>(data, data);

  std::unique_ptr<FarReader<LogArc>> in_reader(
      FarReader<LogArc>::Open(input_far_path_));
  std::unique_ptr<FarReader<LogArc>> out_reader(
      FarReader<LogArc>::Open(output_far_path_));
  ASSERT_NE(in_reader, nullptr);
  ASSERT_NE(out_reader, nullptr);

  VectorFst<LogArc> model;
  const auto s0 = model.AddState();
  model.SetStart(s0);
  model.AddArc(s0, LogArc(1, 1, LogWeight(1.0), s0));
  model.AddArc(s0, LogArc(2, 2, LogWeight(1.0), s0));
  model.SetFinal(s0, LogWeight(0.0));

  const TrainOptions opts(/*max_iters=*/5, /*alpha=*/0.6, /*batch_size=*/2);
  const auto final_likelihood = Train<LogArc>(*in_reader, *out_reader, &model,
                                              /*normalize_ilabel=*/false, opts);

  EXPECT_NE(final_likelihood, LogWeight::Zero());
}

TEST_F(TrainExecutionTest, CorpusLikelihoodSumsExampleNegativeLogLikelihoods) {
  // Four examples: two mapping 1 -> 10 and two mapping 1 -> 20.
  // Under the MLE model P(10 | 1) = P(20 | 1) = 0.5, each example has
  // NLL = -log(0.5) = log(2), so total corpus NLL = 4 * log(2).
  // Accumulating with LogAdder (sum of probabilities) would erroneously yield
  // -log(4 * 0.5) = -log(2) < 0.
  WriteTrainingData<LogArc>({"1", "1", "1", "1"}, {"10", "20", "10", "20"});

  std::unique_ptr<FarReader<LogArc>> in_reader(
      FarReader<LogArc>::Open(input_far_path_));
  std::unique_ptr<FarReader<LogArc>> out_reader(
      FarReader<LogArc>::Open(output_far_path_));
  ASSERT_NE(in_reader, nullptr);
  ASSERT_NE(out_reader, nullptr);

  VectorFst<LogArc> model;
  const auto s0 = model.AddState();
  model.SetStart(s0);
  model.AddArc(s0, LogArc(1, 10, LogWeight(1.0), s0));
  model.AddArc(s0, LogArc(1, 20, LogWeight(1.0), s0));
  model.SetFinal(s0, LogWeight::One());

  const TrainOptions opts(/*max_iters=*/5, /*alpha=*/0.0, /*batch_size=*/0,
                          /*delta=*/1e-5);
  const auto final_likelihood = Train<LogArc>(*in_reader, *out_reader, &model,
                                              /*normalize_ilabel=*/true, opts);

  EXPECT_NEAR(final_likelihood.Value(), 4.0 * std::log(2.0), 1e-4);
}

TEST_F(TrainExecutionTest, SkipsUnalignedExamplesInCorpusLikelihood) {
  // First two examples align (with equal prob 0.5 each -> NLL = 2 * log(2)),
  // third example cannot align (output label 99 absent from model).
  WriteTrainingData<LogArc>({"1", "1", "1"}, {"10", "20", "99"});

  std::unique_ptr<FarReader<LogArc>> in_reader(
      FarReader<LogArc>::Open(input_far_path_));
  std::unique_ptr<FarReader<LogArc>> out_reader(
      FarReader<LogArc>::Open(output_far_path_));
  ASSERT_NE(in_reader, nullptr);
  ASSERT_NE(out_reader, nullptr);

  VectorFst<LogArc> model;
  const auto s0 = model.AddState();
  model.SetStart(s0);
  model.AddArc(s0, LogArc(1, 10, LogWeight(1.0), s0));
  model.AddArc(s0, LogArc(1, 20, LogWeight(1.0), s0));
  model.SetFinal(s0, LogWeight::One());

  const TrainOptions opts(/*max_iters=*/5, /*alpha=*/0.0, /*batch_size=*/0,
                          /*delta=*/1e-5);
  const auto final_likelihood = Train<LogArc>(*in_reader, *out_reader, &model,
                                              /*normalize_ilabel=*/true, opts);

  EXPECT_NEAR(final_likelihood.Value(), 2.0 * std::log(2.0), 1e-4);
}

TEST_F(TrainExecutionTest, NegativeBatchSizeTreatedAsFullBatch) {
  WriteTrainingData<LogArc>({"1", "1"}, {"10", "20"});

  std::unique_ptr<FarReader<LogArc>> in_reader(
      FarReader<LogArc>::Open(input_far_path_));
  std::unique_ptr<FarReader<LogArc>> out_reader(
      FarReader<LogArc>::Open(output_far_path_));
  ASSERT_NE(in_reader, nullptr);
  ASSERT_NE(out_reader, nullptr);

  VectorFst<LogArc> model;
  const auto s0 = model.AddState();
  model.SetStart(s0);
  model.AddArc(s0, LogArc(1, 10, LogWeight(1.0), s0));
  model.AddArc(s0, LogArc(1, 20, LogWeight(1.0), s0));
  model.SetFinal(s0, LogWeight::One());

  const TrainOptions opts(/*max_iters=*/3, /*alpha=*/0.5, /*batch_size=*/-1,
                          /*delta=*/1e-5);
  const auto final_likelihood = Train<LogArc>(*in_reader, *out_reader, &model,
                                              /*normalize_ilabel=*/true, opts);

  EXPECT_NEAR(final_likelihood.Value(), 2.0 * std::log(2.0), 1e-4);
}

TEST_F(TrainExecutionTest,
       ViterbiFindsTrueShortestPathWithMultipleFinalStates) {
  // Acyclic lattice where topological order visits a higher-cost final state
  // (cost 5.0) and a high-cost non-final state (cost 6.0) before the true
  // shortest path (cost 1.5). Passing first_path=true with AutoQueue
  // (TopOrderQueue) breaks out early at cost 5.0 and misses the cheaper path.
  WriteTrainingData<StdArc>({"1"}, {"1"});

  std::unique_ptr<FarReader<StdArc>> in_reader(
      FarReader<StdArc>::Open(input_far_path_));
  std::unique_ptr<FarReader<StdArc>> out_reader(
      FarReader<StdArc>::Open(output_far_path_));
  ASSERT_NE(in_reader, nullptr);
  ASSERT_NE(out_reader, nullptr);

  VectorFst<StdArc> model;
  const auto s0 = model.AddState();
  const auto s1 = model.AddState();  // suboptimal final path (5.0)
  const auto s2 = model.AddState();  // expensive dead branch (6.0)
  const auto s3 = model.AddState();  // cheap branch step 1 (1.0)
  const auto s4 = model.AddState();  // cheap branch final (0.5)
  model.SetStart(s0);
  // Arc order chosen so DFS reverse-finish topological order is
  // s0, s1 (final, 5.0), s2 (6.0), s3 (1.0), s4 (final, 1.5).
  model.AddArc(s0, StdArc(0, 0, TropicalWeight(1.0), s3));
  model.AddArc(s3, StdArc(1, 1, TropicalWeight(0.5), s4));
  model.SetFinal(s4, TropicalWeight::One());
  model.AddArc(s0, StdArc(0, 0, TropicalWeight(6.0), s2));
  model.AddArc(s0, StdArc(1, 1, TropicalWeight(5.0), s1));
  model.SetFinal(s1, TropicalWeight::One());

  // Run one un-normalized batch directly so initial weights (1.0/0.5 vs 5.0 vs
  // 6.0) are preserved in the first E-step.
  internal::ViterbiTrainer<StdArc, StateExpectationTable<StdArc>> trainer(
      /*alpha=*/0.0, /*batch_size=*/0, /*delta=*/1e-5);
  const auto likelihood = trainer.Batch(*in_reader, *out_reader, &model);

  EXPECT_NEAR(likelihood.Value(), 1.5, 1e-4);
  // The chosen path uses s0 -> s3 -> s4 (not s0 -> s1).
  EXPECT_EQ(model.Final(s4), TropicalWeight::One());
  EXPECT_EQ(model.Final(s1), TropicalWeight::Zero());
}

TEST_F(TrainExecutionTest, BaumWelchIgnoresNoWeightErrorLatticeWithoutNan) {
  // First example has a valid alignment; second example has a NoWeight arc
  // causing ShortestDistance to fail. Training must not poison model with NaN.
  {
    std::unique_ptr<FarWriter<LogArc>> in_writer(
        FarWriter<LogArc>::Create(input_far_path_));
    std::unique_ptr<FarWriter<LogArc>> out_writer(
        FarWriter<LogArc>::Create(output_far_path_));
    ASSERT_NE(in_writer, nullptr);
    ASSERT_NE(out_writer, nullptr);

    VectorFst<LogArc> good;
    const auto g0 = good.AddState();
    const auto g1 = good.AddState();
    good.SetStart(g0);
    good.AddArc(g0, LogArc(1, 1, LogWeight::One(), g1));
    good.SetFinal(g1, LogWeight::One());
    in_writer->Add("0", good);
    out_writer->Add("0", good);

    VectorFst<LogArc> bad;
    const auto b0 = bad.AddState();
    const auto b1 = bad.AddState();
    bad.SetStart(b0);
    bad.AddArc(b0, LogArc(1, 1, LogWeight::NoWeight(), b1));
    bad.SetFinal(b1, LogWeight::One());
    in_writer->Add("1", bad);
    out_writer->Add("1", good);
  }

  std::unique_ptr<FarReader<LogArc>> in_reader(
      FarReader<LogArc>::Open(input_far_path_));
  std::unique_ptr<FarReader<LogArc>> out_reader(
      FarReader<LogArc>::Open(output_far_path_));
  ASSERT_NE(in_reader, nullptr);
  ASSERT_NE(out_reader, nullptr);

  VectorFst<LogArc> model;
  const auto s0 = model.AddState();
  model.SetStart(s0);
  model.AddArc(s0, LogArc(1, 1, LogWeight(1.0), s0));
  model.SetFinal(s0, LogWeight::One());

  const TrainOptions opts(/*max_iters=*/2, /*alpha=*/0.0);
  const auto final_likelihood = Train<LogArc>(*in_reader, *out_reader, &model,
                                              /*normalize_ilabel=*/true, opts);
  EXPECT_TRUE(final_likelihood.Member());
  EXPECT_NE(final_likelihood, LogWeight::Zero());
  for (ArcIterator<VectorFst<LogArc>> aiter(model, s0); !aiter.Done();
       aiter.Next()) {
    EXPECT_TRUE(aiter.Value().weight.Member());
  }
}

TEST_F(TrainExecutionTest, PreservesModelSortPropertiesAcrossTraining) {
  WriteTrainingData<LogArc>({"1 2", "2 1"}, {"10 20", "20 10"});

  std::unique_ptr<FarReader<LogArc>> in_reader(
      FarReader<LogArc>::Open(input_far_path_));
  std::unique_ptr<FarReader<LogArc>> out_reader(
      FarReader<LogArc>::Open(output_far_path_));
  ASSERT_NE(in_reader, nullptr);
  ASSERT_NE(out_reader, nullptr);

  VectorFst<LogArc> model;
  const auto s0 = model.AddState();
  model.SetStart(s0);
  model.AddArc(s0, LogArc(2, 10, LogWeight(1.0), s0));
  model.AddArc(s0, LogArc(1, 20, LogWeight(1.0), s0));
  model.AddArc(s0, LogArc(1, 10, LogWeight(1.0), s0));
  model.AddArc(s0, LogArc(2, 20, LogWeight(1.0), s0));
  model.SetFinal(s0, LogWeight::One());
  ArcSort(&model, OLabelCompare<LogArc>());
  ASSERT_NE(model.Properties(kOLabelSorted, false), 0);

  const TrainOptions opts(/*max_iters=*/5, /*alpha=*/0.5, /*batch_size=*/1);
  Train<LogArc>(*in_reader, *out_reader, &model, /*normalize_ilabel=*/true,
                opts);

  EXPECT_NE(model.Properties(kOLabelSorted, false), 0);
}

TEST_F(TrainExecutionTest, DoesNotResetSingleFstInputAcrossIterations) {
  // Counting FarReader wrapper returning FarType::FST to verify Train does not
  // call Reset() on an active single-FST (LM) input across EM iterations.
  class SingleFstReader : public FarReader<LogArc> {
   public:
    explicit SingleFstReader(const VectorFst<LogArc>& fst,
                             bool initially_done = false)
        : fst_(fst), done_(initially_done) {}
    void Reset() override {
      ++reset_calls_;
      done_ = false;
    }
    bool Find(absl::string_view) override { return false; }
    bool Done() const override { return done_; }
    void Next() override { done_ = true; }
    const std::string& GetKey() const override { return key_; }
    const Fst<LogArc>* GetFst() const override { return &fst_; }
    FarType Type() const override { return FarType::FST; }
    bool Error() const override { return false; }
    int reset_calls() const { return reset_calls_; }

   private:
    const VectorFst<LogArc>& fst_;
    std::string key_ = "lm";
    bool done_;
    int reset_calls_ = 0;
  };

  WriteTrainingData<LogArc>({"1"}, {"10"});
  std::unique_ptr<FarReader<LogArc>> out_reader(
      FarReader<LogArc>::Open(output_far_path_));
  ASSERT_NE(out_reader, nullptr);

  VectorFst<LogArc> lm;
  const auto l0 = lm.AddState();
  const auto l1 = lm.AddState();
  lm.SetStart(l0);
  lm.AddArc(l0, LogArc(1, 1, LogWeight::One(), l1));
  lm.SetFinal(l1, LogWeight::One());

  VectorFst<LogArc> model;
  const auto m0 = model.AddState();
  model.SetStart(m0);
  model.AddArc(m0, LogArc(1, 10, LogWeight(0.5), m0));
  model.AddArc(m0, LogArc(1, 20, LogWeight(1.5), m0));
  model.SetFinal(m0, LogWeight::One());

  const TrainOptions opts(/*max_iters=*/5, /*alpha=*/0.5, /*batch_size=*/0,
                          /*delta=*/1e-6);

  // Active FstFarReader should never be Reset() across iterations.
  SingleFstReader active_reader(lm, /*initially_done=*/false);
  Train<LogArc>(active_reader, *out_reader, &model, /*normalize_ilabel=*/true,
                opts);
  EXPECT_EQ(active_reader.reset_calls(), 0);

  // Exhausted FstFarReader should be Reset() once upfront and not again.
  SingleFstReader done_reader(lm, /*initially_done=*/true);
  Train<LogArc>(done_reader, *out_reader, &model, /*normalize_ilabel=*/true,
                opts);
  EXPECT_EQ(done_reader.reset_calls(), 1);
}

}  // namespace
}  // namespace fst
