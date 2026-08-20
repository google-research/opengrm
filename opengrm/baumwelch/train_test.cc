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

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/extensions/far/far-reader.h"
#include "openfst/extensions/far/far-writer.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
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

}  // namespace
}  // namespace fst
