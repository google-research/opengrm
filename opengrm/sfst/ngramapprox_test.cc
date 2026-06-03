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

// Unit tests for n-gram approximation for stochastic FSTs.

#include "opengrm/sfst/ngramapprox.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/relabel.h"
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

class NGramApproxTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string approx1_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/approx1.fst");
    const std::string approx2_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/approx2.fst");
    const std::string approx3_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/approx3.fst");
    const std::string approx4_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/approx4.fst");
    const std::string approx5_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/approx5.fst");
    const std::string approx6_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/approx6.fst");

    afst1_.reset(fst::VectorFst<Arc>::Read(approx1_name));
    afst2_.reset(fst::VectorFst<Arc>::Read(approx2_name));
    afst3_.reset(fst::VectorFst<Arc>::Read(approx3_name));
    afst4_.reset(fst::VectorFst<Arc>::Read(approx4_name));
    afst5_.reset(fst::VectorFst<Arc>::Read(approx5_name));
    afst6_.reset(fst::VectorFst<Arc>::Read(approx6_name));
  }

  std::unique_ptr<fst::VectorFst<Arc>> afst1_;
  std::unique_ptr<fst::VectorFst<Arc>> afst2_;
  std::unique_ptr<fst::VectorFst<Arc>> afst3_;
  std::unique_ptr<fst::VectorFst<Arc>> afst4_;
  std::unique_ptr<fst::VectorFst<Arc>> afst5_;
  std::unique_ptr<fst::VectorFst<Arc>> afst6_;

  // Algorithm delta
  static constexpr float kAlgoDelta = 1.0e-5;

  // Comparison delta
  static constexpr float kCmpDelta = 0.01;
};

// Acyclic input with one failure transition
TEST_F(NGramApproxTest, AcyclicBiApprox) {
  ASSERT_TRUE(IsNormalized(*afst1_, 0));
  fst::VectorFst<Arc> afst2;
  NGramApprox(*afst1_, &afst2, 2, 0, kAlgoDelta, NORM_SUMMED);
  ASSERT_TRUE(fst::Verify(afst2));
  ASSERT_TRUE(Equal(*afst2_, afst2, 0, kCmpDelta));
}

// Cyclic input with one failure transition
TEST_F(NGramApproxTest, CyclicBiApprox) {
  ASSERT_TRUE(IsNormalized(*afst3_, 0));
  fst::VectorFst<Arc> afst4;
  NGramApprox(*afst3_, &afst4, 2, 0, kAlgoDelta, NORM_SUMMED);
  ASSERT_TRUE(fst::Verify(afst4));
  ASSERT_TRUE(Equal(*afst4_, afst4, 0, kCmpDelta));
}

// Acyclic input with two failure transitions in a path
TEST_F(NGramApproxTest, AcyclicTriApprox) {
  ASSERT_TRUE(IsNormalized(*afst5_, 0));
  fst::VectorFst<Arc> afst6;
  NGramApprox(*afst5_, &afst6, 3, 0, kAlgoDelta, NORM_SUMMED);
  ASSERT_TRUE(fst::Verify(afst6));
  ASSERT_TRUE(Equal(*afst6_, afst6, 0, kCmpDelta));
}

// Acyclic input with two failure transitions in a path
// where phi_label is non-zero
TEST_F(NGramApproxTest, NonZeroPhiApprox) {
  const Label phi_label = -3;
  fst::VectorFst<Arc> afst5(*afst5_);
  std::vector<std::pair<Label, Label>> pairs = {{0, phi_label}};
  fst::Relabel(&afst5, pairs, pairs);
  ASSERT_TRUE(IsNormalized(afst5, phi_label));
  fst::VectorFst<Arc> afst6;
  NGramApprox(afst5, &afst6, 3, phi_label, kAlgoDelta, NORM_SUMMED);
  // Relabels backoff
  std::vector<std::pair<Label, Label>> relabel_pairs = {{phi_label, 0}};
  fst::Relabel(&afst6, relabel_pairs, relabel_pairs);
  ASSERT_TRUE(fst::Verify(afst6));
  ASSERT_TRUE(Equal(*afst6_, afst6, 0, kCmpDelta));
}

}  // namespace sfst

int main(int argc, char** argv) {
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
