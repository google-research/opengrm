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

// Unit tests for normalization operations and checks for stochastic FSTs.

#include "opengrm/sfst/shortest-distance.h"

#include <sstream>
#include <vector>

#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/signed-log-weight.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/compile-impl.h"

namespace sfst {

typedef fst::StdArc Arc;
typedef Arc::StateId StateId;
typedef Arc::Weight Weight;
typedef Arc::Label Label;

const float kAlgoDelta = 1.0e-7;  // Algorithm delta

// Test class.
class ShortestDistanceTest : public ::testing::Test {};

TEST_F(ShortestDistanceTest, SLTest) {
  using Arc = fst::SignedLog64Arc;
  using Weight = Arc::Weight;
  std::istringstream sfst_strm(
      "0 1 1 -1,0.0 \n"       // 1 - 2 = -1
      "0 2 0 1,-0.693147 \n"  // 2
      "1 0 1 1,0.693147 \n"   // 0.5
      "2 1 1 1,0.0 \n"        // 1
      "1\n");
  fst::FstCompiler<Arc> sfst_comp(sfst_strm, "", nullptr, nullptr, nullptr,
                                  true, false, false, true);
  fst::VectorFst<Arc> sfst(sfst_comp.Fst());
  ASSERT_TRUE(fst::Verify(sfst));

  internal::SignedShortestDistance<Arc> sdist(&sfst, 0, kAlgoDelta);
  std::vector<Weight> distance;
  sdist.ComputeDistance(&distance, false);
  Weight w0(Weight::W1(1.0), Weight::W2(-0.69315));
  Weight w1(Weight::W1(1.0), Weight::W2(-0.69315));
  Weight w2(Weight::W1(1.0), Weight::W2(-1.38629));
  ASSERT_TRUE(ApproxEqual(distance[0], w0));
  ASSERT_TRUE(ApproxEqual(distance[1], w1));
  ASSERT_TRUE(ApproxEqual(distance[2], w2));
}

TEST_F(ShortestDistanceTest, ReverseSLTest) {
  using Arc = fst::SignedLog64Arc;
  using Weight = Arc::Weight;
  std::istringstream sfst_strm(
      "0 1 1 -1,0.0 \n"       // 1 - 2 = -1
      "0 2 0 1,-0.693147 \n"  // 2
      "1 0 1 1,0.693147 \n"   // 0.5
      "2 1 1 1,0.0 \n"        // 1
      "1\n");
  fst::FstCompiler<Arc> sfst_comp(sfst_strm, "", nullptr, nullptr, nullptr,
                                  true, false, false, true);
  fst::VectorFst<Arc> sfst(sfst_comp.Fst());
  ASSERT_TRUE(fst::Verify(sfst));

  internal::SignedShortestDistance<Arc> sdist(&sfst, 0, kAlgoDelta);
  std::vector<Weight> distance;
  sdist.ComputeDistance(&distance, true);
  Weight w0(typename Weight::W1(1.0), typename Weight::W2(-0.69315));
  Weight w1(typename Weight::W1(1.0), typename Weight::W2(-0.69315));
  Weight w2(typename Weight::W1(1.0), typename Weight::W2(-0.69315));
  ASSERT_TRUE(ApproxEqual(distance[0], w0));
  ASSERT_TRUE(ApproxEqual(distance[1], w1));
  ASSERT_TRUE(ApproxEqual(distance[2], w2));
}

TEST_F(ShortestDistanceTest, PhiTest) {
  using Arc = fst::StdArc;
  using Weight = Arc::Weight;
  std::istringstream sfst_strm(
      "0 1 1\n"
      "0 2 2\n"
      "1 3 2\n"
      "2\n"
      "3\n");
  fst::FstCompiler<Arc> sfst_comp(sfst_strm, "", nullptr, nullptr, nullptr,
                                  true, false, false, true);
  fst::VectorFst<Arc> sfst(sfst_comp.Fst());
  ASSERT_TRUE(fst::Verify(sfst));

  std::vector<Weight> distance;
  ShortestDistance(sfst, &distance, 1, false, kAlgoDelta);
  ASSERT_TRUE(ApproxEqual(distance[0], Weight::One()));
  ASSERT_TRUE(ApproxEqual(distance[1], Weight::One()));
  ASSERT_TRUE(ApproxEqual(distance[2], Weight::One()));
  ASSERT_TRUE(ApproxEqual(distance[3], Weight::Zero()));
}

TEST_F(ShortestDistanceTest, PhiReverseTest) {
  using Arc = fst::StdArc;
  using Weight = Arc::Weight;
  std::istringstream sfst_strm(
      "0 1 1\n"
      "0 2 2\n"
      "1 3 2\n"
      "2\n"
      "3\n");
  fst::FstCompiler<Arc> sfst_comp(sfst_strm, "", nullptr, nullptr, nullptr,
                                  true, false, false, true);
  fst::VectorFst<Arc> sfst(sfst_comp.Fst());
  ASSERT_TRUE(fst::Verify(sfst));

  std::vector<Weight> distance;
  ShortestDistance(sfst, &distance, 1, true, kAlgoDelta);
  ASSERT_TRUE(ApproxEqual(distance[0], Weight::One()));
  ASSERT_TRUE(ApproxEqual(distance[1], Weight::One()));
  ASSERT_TRUE(ApproxEqual(distance[2], Weight::One()));
  ASSERT_TRUE(ApproxEqual(distance[3], Weight::One()));
}

}  // namespace sfst

int main(int argc, char** argv) {
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
