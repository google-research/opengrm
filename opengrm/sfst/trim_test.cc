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

// Unit tests for stochastic FST trimming algorithm.

#include "opengrm/sfst/trim.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "opengrm/sfst/backoff.h"

namespace sfst {

typedef fst::StdArc Arc;
typedef Arc::StateId StateId;
typedef Arc::Weight Weight;
typedef Arc::Label Label;

class TrimTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string trim1_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/trim1.fst");
    const std::string trim2_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/trim2.fst");
    const std::string trim3_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/trim3.fst");
    const std::string trim4_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/trim4.fst");
    const std::string trim5_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/trim5.fst");
    const std::string trim6_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/trim6.fst");

    tfst1_.reset(fst::VectorFst<Arc>::Read(trim1_name));
    tfst2_.reset(fst::VectorFst<Arc>::Read(trim2_name));
    tfst3_.reset(fst::VectorFst<Arc>::Read(trim3_name));
    tfst4_.reset(fst::VectorFst<Arc>::Read(trim4_name));
    tfst5_.reset(fst::VectorFst<Arc>::Read(trim5_name));
    tfst6_.reset(fst::VectorFst<Arc>::Read(trim6_name));
  }

  std::unique_ptr<fst::VectorFst<Arc>> tfst1_;
  std::unique_ptr<fst::VectorFst<Arc>> tfst2_;
  std::unique_ptr<fst::VectorFst<Arc>> tfst3_;
  std::unique_ptr<fst::VectorFst<Arc>> tfst4_;
  std::unique_ptr<fst::VectorFst<Arc>> tfst5_;
  std::unique_ptr<fst::VectorFst<Arc>> tfst6_;
};

// Input is connected (when phi_label=1 treated as a regular symbol)
TEST_F(TrimTest, ConnectedTest) {
  ASSERT_FALSE(IsTrim(*tfst1_, 1));
  fst::VectorFst<Arc> tfst2(*tfst1_);
  Trim(&tfst2, 1);
  ASSERT_TRUE(fst::Verify(tfst2));
  ASSERT_TRUE(IsTrim(tfst2, 1));
  ASSERT_TRUE(fst::Equal(*tfst2_, tfst2));
}

// Input is disconnected
TEST_F(TrimTest, DisconnectedTest) {
  ASSERT_FALSE(IsTrim(*tfst3_, 1));
  fst::VectorFst<Arc> tfst4(*tfst3_);
  Trim(&tfst4, 1);
  ASSERT_TRUE(fst::Verify(tfst4));
  ASSERT_TRUE(IsTrim(tfst4, 1));
  ASSERT_TRUE(fst::Equal(*tfst4_, tfst4));
}

// Input has needed non-coaccessible transition.
TEST_F(TrimTest, NeededTest) {
  ASSERT_FALSE(IsTrim(*tfst5_, 0));
  fst::VectorFst<Arc> tfst6(*tfst5_);
  Trim(&tfst6, 0);
  ASSERT_TRUE(fst::Verify(tfst6));
  ASSERT_TRUE(IsTrim(tfst6, 0));
  ASSERT_TRUE(fst::Equal(*tfst6_, tfst6));
}

// Weight trimming
// Checks SumBackoff + WeightTrim and
// SumWeightTrim have same topology
TEST_F(TrimTest, WeightTest) {
  fst::VectorFst<Arc> fst(*tfst1_);

  Trimmer<Arc> trimmer(&fst, 1);
  trimmer.SumWeightTrim(false, typename Arc::Weight(0.9));
  trimmer.Finalize();

  fst::VectorFst<Arc> sum_fst(*tfst1_);
  SumBackoff(&sum_fst, 1);
  Trimmer<Arc> sum_trimmer(&sum_fst, 1);
  sum_trimmer.WeightTrim(false, typename Arc::Weight(0.9));
  sum_trimmer.Finalize();

  fst::RmWeightMapper<Arc> rm_weight_mapper;
  fst::ArcMap(&fst, rm_weight_mapper);
  fst::ArcMap(&sum_fst, rm_weight_mapper);
  ASSERT_TRUE(fst::Equal(fst, sum_fst));
}

}  // namespace sfst

int main(int argc, char** argv) {
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
