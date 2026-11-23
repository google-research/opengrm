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

// Enable --logtostderr --v=1 to see the actual number of states explored.

#include "opengrm/baumwelch/a-star.h"

#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/log/check.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

class AStarTest : public ::testing::Test {
 protected:
  void SetUp() final {
    const std::string path =
        fst::JoinPath(std::string("."),
                       "opengrm/baumwelch/testdata/a-star");
    const std::string nfa_name = fst::JoinPath(path, "nfa.fst");
    const std::string shortest_name = fst::JoinPath(path, "shortest.fst");

    nfa_.reset(VectorFst<LogArc>::Read(nfa_name));
    CHECK(nfa_ != nullptr);
    shortest_.reset(VectorFst<LogArc>::Read(shortest_name));
    CHECK(shortest_ != nullptr);
  }

  std::unique_ptr<VectorFst<LogArc>> nfa_;
  std::unique_ptr<VectorFst<LogArc>> shortest_;
};

TEST_F(AStarTest, AStarTest) {
  VectorFst<LogArc> shortest;
  AStarSingleShortestString(*nfa_, &shortest);

  EXPECT_TRUE(Equal(*shortest_, shortest));
}

TEST_F(AStarTest, SupportsInPlaceAliasing) {
  VectorFst<LogArc> fst(*nfa_);
  AStarSingleShortestString(fst, &fst);

  EXPECT_TRUE(Equal(*shortest_, fst));
}

TEST(AStarEdgeCaseTest, ConvertingEstimateOutOfBoundsAndNegativeState) {
  const std::vector<LogWeight> beta = {LogWeight(1.5f), LogWeight(2.5f)};
  const ConvertingAStarEstimate<LogArc::StateId, TropicalWeight, LogWeight>
      estimate(beta);
  EXPECT_EQ(estimate(0), TropicalWeight(1.5f));
  EXPECT_EQ(estimate(1), TropicalWeight(2.5f));
  EXPECT_EQ(estimate(2), TropicalWeight::Zero());
  EXPECT_EQ(estimate(kNoStateId), TropicalWeight::Zero());
}

TEST(AStarEdgeCaseTest, EmptyFstReturnsEmptyWithoutError) {
  VectorFst<LogArc> empty;
  VectorFst<LogArc> shortest;
  // Pre-populate output to ensure stale states are cleared on early return.
  const auto stale = shortest.AddState();
  shortest.SetStart(stale);
  shortest.SetFinal(stale, LogWeight::One());

  AStarSingleShortestString(empty, &shortest);
  EXPECT_EQ(shortest.Start(), kNoStateId);
  EXPECT_EQ(shortest.NumStates(), 0);
  EXPECT_EQ(shortest.Properties(kError, false), 0);
}

TEST(AStarEdgeCaseTest, NonCoaccessibleFstReturnsEmptyWithoutError) {
  VectorFst<LogArc> nfa;
  const auto s0 = nfa.AddState();
  const auto s1 = nfa.AddState();
  nfa.SetStart(s0);
  nfa.AddArc(s0, LogArc(1, 1, LogWeight(1.0f), s1));
  // No final state.
  VectorFst<LogArc> shortest;
  AStarSingleShortestString(nfa, &shortest);
  EXPECT_EQ(shortest.Start(), kNoStateId);
  EXPECT_EQ(shortest.Properties(kError, false), 0);
}

TEST(AStarEdgeCaseTest, PropagatesInputError) {
  VectorFst<LogArc> bad;
  bad.SetProperties(kError, kError);
  VectorFst<LogArc> shortest;
  AStarSingleShortestString(bad, &shortest);
  EXPECT_NE(shortest.Properties(kError, false), 0);
}

TEST(AStarEdgeCaseTest, PropagatesShortestDistanceError) {
  VectorFst<LogArc> bad;
  const auto s0 = bad.AddState();
  const auto s1 = bad.AddState();
  bad.SetStart(s0);
  bad.AddArc(s0, LogArc(1, 1, LogWeight::NoWeight(), s1));
  bad.SetFinal(s1, LogWeight::One());

  VectorFst<LogArc> shortest;
  AStarSingleShortestString(bad, &shortest);
  EXPECT_NE(shortest.Properties(kError, false), 0);
}

TEST(AStarEdgeCaseTest, HonorsFineDeltaInDeterminization) {
  // Two competing paths sharing prefix label 1 whose residual weights at the
  // NFA subset differ by less than kDelta (1/1024 ~= 9.77e-4). With coarse
  // kDelta quantization, both residuals quantize identically and label-order
  // tie-breaking picks the suboptimal "1 2"; with delta=1e-6, "1 3" wins.
  VectorFst<LogArc> nfa;
  const auto s0 = nfa.AddState();
  const auto s1a = nfa.AddState();
  const auto s1b = nfa.AddState();
  const auto s2 = nfa.AddState();
  nfa.SetStart(s0);
  nfa.AddArc(s0, LogArc(1, 1, LogWeight(1.0002f), s1a));
  nfa.AddArc(s0, LogArc(1, 1, LogWeight(1.0f), s1b));
  nfa.AddArc(s1a, LogArc(2, 2, LogWeight(1.0f), s2));
  nfa.AddArc(s1b, LogArc(3, 3, LogWeight(1.0f), s2));
  nfa.SetFinal(s2, LogWeight::One());

  VectorFst<LogArc> shortest;
  AStarSingleShortestString(nfa, &shortest, /*delta=*/1e-6f);
  ASSERT_EQ(shortest.Properties(kError, false), 0);
  ASSERT_NE(shortest.Start(), kNoStateId);
  // SingleShortestPathBacktrace numbers states in reverse (start -> ... -> 0).
  const auto first_arc =
      ArcIterator<VectorFst<LogArc>>(shortest, shortest.Start()).Value();
  EXPECT_EQ(first_arc.ilabel, 1);
  const auto second_arc =
      ArcIterator<VectorFst<LogArc>>(shortest, first_arc.nextstate).Value();
  EXPECT_EQ(second_arc.ilabel, 3);
}

}  // namespace
}  // namespace fst
