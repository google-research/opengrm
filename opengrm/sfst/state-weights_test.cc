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

// Unit tests for state weight utilities in stochastic FSTs.

#include "opengrm/sfst/state-weights.h"

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/base/log_severity.h"
#include "absl/flags/flag.h"
#include "absl/log/globals.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/signed-log-weight.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/util.h"
#include "openfst/lib/vector-fst.h"

namespace sfst {
namespace {

using fst::Log64Weight;
using fst::SignedLog64Arc;
using fst::SignedLog64Weight;
using fst::StdArc;
using fst::VectorFst;

TEST(StateWeightsTest, ApproxEqualPredLogWeight) {
  internal::ApproxEqualPred<Log64Weight> pred(1e-3);

  // Exact equality
  EXPECT_TRUE(pred(Log64Weight(1.5), Log64Weight(1.5)));

  // Within delta
  EXPECT_TRUE(pred(Log64Weight(1.5), Log64Weight(1.5 + 1e-4)));

  // Outside delta
  EXPECT_FALSE(pred(Log64Weight(1.5), Log64Weight(1.5 + 0.1)));

  // Both approximately zero (probability < exp(-99.0) by default)
  EXPECT_TRUE(pred(Log64Weight(100.0), Log64Weight(120.0)));

  // One approximately zero and the other not
  EXPECT_FALSE(pred(Log64Weight(10.0), Log64Weight(100.0)));
}

TEST(StateWeightsTest, ApproxEqualPredSignedLogWeight) {
  internal::ApproxEqualPred<SignedLog64Weight> pred(1e-3);

  SignedLog64Weight w1(SignedLog64Weight::W1(1.0), SignedLog64Weight::W2(1.5));
  SignedLog64Weight w2(SignedLog64Weight::W1(1.0),
                       SignedLog64Weight::W2(1.5 + 1e-4));
  SignedLog64Weight w3(SignedLog64Weight::W1(1.0), SignedLog64Weight::W2(2.0));
  SignedLog64Weight z1(SignedLog64Weight::W1(1.0),
                       SignedLog64Weight::W2(100.0));
  SignedLog64Weight z2(SignedLog64Weight::W1(-1.0),
                       SignedLog64Weight::W2(110.0));

  EXPECT_TRUE(pred(w1, w1));
  EXPECT_TRUE(pred(w1, w2));
  EXPECT_FALSE(pred(w1, w3));
  EXPECT_TRUE(pred(z1, z2));
}

TEST(StateWeightsTest, NormWeights) {
  std::vector<Log64Weight> weights = {Log64Weight(-std::log(2.0)),
                                      Log64Weight(-std::log(6.0))};
  NormWeights(&weights);
  ASSERT_EQ(weights.size(), 2);
  // Normalized values: 2/(2+6) = 0.25, 6/(2+6) = 0.75
  EXPECT_NEAR(weights[0].Value(), -std::log(0.25), 1e-5);
  EXPECT_NEAR(weights[1].Value(), -std::log(0.75), 1e-5);
}

TEST(StateWeightsTest, ApproxEqualWeights) {
  std::vector<Log64Weight> v1 = {Log64Weight(1.0), Log64Weight(2.0)};
  std::vector<Log64Weight> v2 = {Log64Weight(1.0 + 1e-6),
                                 Log64Weight(2.0 - 1e-6)};
  std::vector<Log64Weight> v3 = {Log64Weight(1.0), Log64Weight(2.5)};
  std::vector<Log64Weight> v4 = {Log64Weight(1.0)};

  EXPECT_TRUE(ApproxEqualWeights(v1, v2));
  EXPECT_FALSE(ApproxEqualWeights(v1, v3));
  EXPECT_FALSE(ApproxEqualWeights(v1, v4));
}

TEST(StateWeightsTest, WriteWeightsToStream) {
  std::vector<Log64Weight> weights = {Log64Weight(0.5), Log64Weight(1.25)};
  std::ostringstream strm;
  WriteWeights(strm, weights);
  EXPECT_EQ(strm.str(), "0\t0.5\n1\t1.25\n");
}

TEST(StateWeightsTest, WriteWeightsToFile) {
  std::vector<Log64Weight> weights = {Log64Weight(0.5), Log64Weight(1.25)};
  const std::string filename = fst::JoinPath(
      absl::GetFlag(::testing::TempDir()), "state_weights_test.txt");
  EXPECT_TRUE(WriteWeights(filename, weights));

  EXPECT_FALSE(
      WriteWeights("/nonexistent_dir_for_sfst_test/test.txt", weights));
}

TEST(StateWeightsTest, SumAndDiffStateWeightsWithFailArc) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);

  const StdArc::Label kPhiLabel = 10;
  // 0 -> 1 via phi
  fst.AddArc(0,
             StdArc(kPhiLabel, kPhiLabel, StdArc::Weight(-std::log(0.5)), 1));
  // 1 -> 2 via phi
  fst.AddArc(1,
             StdArc(kPhiLabel, kPhiLabel, StdArc::Weight(-std::log(0.2)), 2));
  fst.SetFinal(2, StdArc::Weight::One());

  std::vector<StdArc::Weight> weights = {StdArc::Weight(-std::log(0.4)),
                                         StdArc::Weight(-std::log(0.3)),
                                         StdArc::Weight(-std::log(0.3))};

  SumStateWeights(fst, &weights, kPhiLabel, /*fail_arc=*/true);
  ASSERT_EQ(weights.size(), 3);
  // State 1 receives 0.4 * 0.5 = 0.2 from State 0. Total = 0.3 + 0.2 = 0.5
  EXPECT_NEAR(weights[1].Value(), -std::log(0.5), 1e-4);
  // State 2 receives 0.5 * 0.2 = 0.1 from State 1. Total = 0.3 + 0.1 = 0.4
  EXPECT_NEAR(weights[2].Value(), -std::log(0.4), 1e-4);

  // Now diff should recover exact initial weights
  DiffStateWeights(fst, &weights, kPhiLabel, /*fail_arc=*/true);
  EXPECT_NEAR(weights[0].Value(), -std::log(0.4), 1e-4);
  EXPECT_NEAR(weights[1].Value(), -std::log(0.3), 1e-4);
  EXPECT_NEAR(weights[2].Value(), -std::log(0.3), 1e-4);
}

TEST(StateWeightsTest, SumAndDiffStateWeightsWithoutFailArc) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);

  const StdArc::Label kPhiLabel = 10;
  fst.AddArc(0,
             StdArc(kPhiLabel, kPhiLabel, StdArc::Weight(-std::log(0.5)), 1));
  fst.SetFinal(1, StdArc::Weight::One());

  std::vector<StdArc::Weight> weights = {StdArc::Weight(-std::log(0.4)),
                                         StdArc::Weight(-std::log(0.3))};

  SumStateWeights(fst, &weights, kPhiLabel, /*fail_arc=*/false);
  ASSERT_EQ(weights.size(), 2);
  // When fail_arc is false, entire weight[0] (0.4) is added to weight[1] (0.3)
  EXPECT_NEAR(weights[1].Value(), -std::log(0.7), 1e-4);

  DiffStateWeights(fst, &weights, kPhiLabel, /*fail_arc=*/false);
  EXPECT_NEAR(weights[0].Value(), -std::log(0.4), 1e-4);
  EXPECT_NEAR(weights[1].Value(), -std::log(0.3), 1e-4);
}

TEST(StateWeightsTest, SumAndDiffStateWeightsNoLabel) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.SetStart(0);

  std::vector<StdArc::Weight> weights = {StdArc::Weight(1.0)};
  SumStateWeights(fst, &weights, fst::kNoLabel, true);
  EXPECT_EQ(weights[0].Value(), 1.0);

  DiffStateWeights(fst, &weights, fst::kNoLabel, true);
  EXPECT_EQ(weights[0].Value(), 1.0);
}

TEST(StateWeightsTest, SumAndDiffStateWeightsCyclicFst) {
  const bool orig_fatal = absl::GetFlag(FLAGS_fst_error_fatal);
  absl::SetFlag(&FLAGS_fst_error_fatal, false);

  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);

  const StdArc::Label kPhiLabel = 10;
  fst.AddArc(0, StdArc(kPhiLabel, kPhiLabel, StdArc::Weight::One(), 1));
  fst.AddArc(1, StdArc(kPhiLabel, kPhiLabel, StdArc::Weight::One(), 0));

  std::vector<StdArc::Weight> weights = {StdArc::Weight(1.0),
                                         StdArc::Weight(2.0)};
  SumStateWeights(fst, &weights, kPhiLabel, true);
  EXPECT_EQ(weights[0].Value(), 1.0);
  EXPECT_EQ(weights[1].Value(), 2.0);

  DiffStateWeights(fst, &weights, kPhiLabel, true);
  EXPECT_EQ(weights[0].Value(), 1.0);
  EXPECT_EQ(weights[1].Value(), 2.0);

  absl::SetFlag(&FLAGS_fst_error_fatal, orig_fatal);
}

TEST(StateWeightsTest, SumStates) {
  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);

  // State 0: two outgoing arcs with prob 0.4 each, final weight prob 0.2
  fst.AddArc(0, StdArc(1, 1, StdArc::Weight(-std::log(0.4)), 1));
  fst.AddArc(0, StdArc(2, 2, StdArc::Weight(-std::log(0.4)), 2));
  fst.SetFinal(0, StdArc::Weight(-std::log(0.2)));

  // State 1: one outgoing arc prob 0.5, final weight prob 0.5
  fst.AddArc(1, StdArc(2, 2, StdArc::Weight(-std::log(0.5)), 2));
  fst.SetFinal(1, StdArc::Weight(-std::log(0.5)));

  // State 2: no outgoing arcs, final weight prob 0.25
  fst.SetFinal(2, StdArc::Weight(-std::log(0.25)));

  std::vector<StdArc::Weight> weights;
  SumStates(fst, /*phi_label=*/0, &weights);
  ASSERT_EQ(weights.size(), 3);
  EXPECT_NEAR(weights[0].Value(), 0.0, 1e-4);
  EXPECT_NEAR(weights[1].Value(), 0.0, 1e-4);
  EXPECT_NEAR(weights[2].Value(), -std::log(0.25), 1e-4);
}

TEST(StateWeightsTest, SumStatesCyclicFst) {
  bool orig_fatal = absl::GetFlag(FLAGS_fst_error_fatal);
  absl::SetFlag(&FLAGS_fst_error_fatal, false);

  VectorFst<StdArc> fst;
  fst.AddState();
  fst.AddState();
  fst.SetStart(0);

  fst.AddArc(0, StdArc(0, 0, StdArc::Weight::One(), 1));
  fst.AddArc(1, StdArc(0, 0, StdArc::Weight::One(), 0));

  std::vector<StdArc::Weight> weights;
  SumStates(fst, /*phi_label=*/0, &weights);
  EXPECT_TRUE(weights.empty());

  absl::SetFlag(&FLAGS_fst_error_fatal, orig_fatal);
}

}  // namespace
}  // namespace sfst

int main(int argc, char** argv) {
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
