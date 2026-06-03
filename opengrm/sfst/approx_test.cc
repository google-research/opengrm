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

// Unit tests for approximation of stochastic FSTs.

#include "opengrm/sfst/approx.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "opengrm/sfst/equal.h"
#include "opengrm/sfst/normalize.h"

namespace sfst {

typedef fst::StdArc Arc;
typedef Arc::StateId StateId;
typedef Arc::Weight Weight;
typedef Arc::Label Label;

class ApproxTest : public testing::Test {
 protected:
  void SetUp() override {
    // Acyclic input SFST with two failure transitions in a path.
    const std::string sfst1_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/approx7.fst");
    // Golden approximation of SFST1 (phi-summed).
    const std::string approx1_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/approx8.fst");
    // Cyclic input SFST and 3-gram topology FST
    const std::string sfst2_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/approx9.fst");
    // Golden approximation of SFST2 (marg-constrained).
    const std::string approx2_name = fst::JoinPath(
        std::string("."),
        "opengrm/sfst/testdata/approx10.fst");

    sfst1_.reset(fst::VectorFst<Arc>::Read(sfst1_name));
    approx1_.reset(fst::VectorFst<Arc>::Read(approx1_name));
    sfst2_.reset(fst::VectorFst<Arc>::Read(sfst2_name));
    approx2_.reset(fst::VectorFst<Arc>::Read(approx2_name));
  }

  std::unique_ptr<fst::VectorFst<Arc>> sfst1_;
  std::unique_ptr<fst::VectorFst<Arc>> top1_;
  std::unique_ptr<fst::VectorFst<Arc>> approx1_;
  std::unique_ptr<fst::VectorFst<Arc>> sfst2_;
  std::unique_ptr<fst::VectorFst<Arc>> approx2_;

  // Algorithm delta
  static constexpr float kAlgoDelta = 1.0e-6;

  // Comparison delta
  static constexpr float kCmpDelta = 0.01;
};

// Acyclic input with two failure transitions in a path
// w/ phi-summed output.
TEST_F(ApproxTest, AcyclicTriApprox) {
  ASSERT_TRUE(IsNormalized(*sfst1_, 0));
  fst::VectorFst<Arc> approx1(*sfst1_);

  // Computes approximation of sfst1 wrt same topology.
  Approx(*sfst1_, &approx1, 0, kAlgoDelta, NORM_SUMMED);
  ASSERT_TRUE(fst::Verify(approx1));
  // Compares to golden result.
  ASSERT_TRUE(Equal(*approx1_, approx1, 0, kCmpDelta));
}

// Acyclic input with two failure transitions in a path
// w/ marg-constrained output.
TEST_F(ApproxTest, MCAcyclicTriApprox) {
  ASSERT_TRUE(IsNormalized(*sfst1_, 0));
  fst::VectorFst<Arc> approx1(*sfst1_);

  // Computes approximation of sfst1 wrt same topology.
  Approx(*sfst1_, &approx1, 0, kAlgoDelta, NORM_KL_MIN);

  ASSERT_TRUE(fst::Verify(approx1));
  // Compares to input; approx should be identity op.
  ASSERT_TRUE(Equal(*sfst1_, approx1, 0, kCmpDelta));
}

// Cyclic trigram input w/ phi-summed output.
TEST_F(ApproxTest, CyclicTriApprox) {
  ASSERT_TRUE(IsNormalized(*sfst2_, 0));
  fst::VectorFst<Arc> approx2(*sfst2_);

  // Computes approximation of sfst1 wrt same topology.
  Approx(*sfst2_, &approx2, 0, kAlgoDelta, NORM_SUMMED);
  ASSERT_TRUE(fst::Verify(approx2));
  // Compares to golden result.
  ASSERT_TRUE(Equal(*approx2_, approx2, 0, kCmpDelta));
}

// Cyclic trigram input w/ marg-constrained output.
TEST_F(ApproxTest, MCCyclicTriApprox) {
  ASSERT_TRUE(IsNormalized(*sfst2_, 0));
  fst::VectorFst<Arc> approx2(*sfst2_);

  // Computes approximation of sfst1 wrt same topology.
  Approx(*sfst2_, &approx2, 0, kAlgoDelta, NORM_KL_MIN);

  ASSERT_TRUE(fst::Verify(approx2));
  // Compares to input; approx should be identity op.
  ASSERT_TRUE(Equal(*sfst2_, approx2, 0, kCmpDelta));
}

}  // namespace sfst

int main(int argc, char** argv) {
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
