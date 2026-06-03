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

// Unit tests for stochastic FST topology-induction algorithms.

#include "opengrm/sfst/topology.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/sfst/canonical.h"

namespace sfst {

typedef fst::StdArc Arc;
typedef Arc::StateId StateId;
typedef Arc::Weight Weight;
typedef Arc::Label Label;

class TopologyTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string topology1_name = fst::JoinPath(
        std::string("."),
        "opengrm/sfst/testdata/topology1.fst");
    const std::string topology2_name = fst::JoinPath(
        std::string("."),
        "opengrm/sfst/testdata/topology2.fst");

    tfst1_.reset(fst::VectorFst<Arc>::Read(topology1_name));
    tfst2_.reset(fst::VectorFst<Arc>::Read(topology2_name));
  }

  std::unique_ptr<fst::VectorFst<Arc>> tfst1_;
  std::unique_ptr<fst::VectorFst<Arc>> tfst2_;
};

TEST_F(TopologyTest, NGramTopologyTest) {
  fst::VectorFst<Arc> tfst2;
  NGramTopology<Arc> ngram(3, 0, &tfst2);
  ngram.FindNGrams(*tfst1_);
  ASSERT_TRUE(IsCanonical(tfst2, 0));
  ASSERT_TRUE(fst::Equal(*tfst2_, tfst2));
}

}  // namespace sfst

int main(int argc, char** argv) {
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
