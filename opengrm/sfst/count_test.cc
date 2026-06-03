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

// Unit tests for stochastic FST counting algorithm.

#include "opengrm/sfst/count.h"

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

class CountTest : public testing::Test {
 protected:
  void SetUp() override {
    const std::string count1_name = fst::JoinPath(
        std::string("."),
        "opengrm/sfst/testdata/topology1.fst");
    const std::string count2_name = fst::JoinPath(
        std::string("."),
        "opengrm/sfst/testdata/topology2.fst");
    const std::string count3_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/count2.fst");

    cfst1_.reset(fst::VectorFst<Arc>::Read(count1_name));
    cfst2_.reset(fst::VectorFst<Arc>::Read(count2_name));
    cfst3_.reset(fst::VectorFst<Arc>::Read(count3_name));
  }

  static constexpr float kAlgoDelta = 0.0001;

  std::unique_ptr<fst::VectorFst<Arc>> cfst1_;
  std::unique_ptr<fst::VectorFst<Arc>> cfst2_;
  std::unique_ptr<fst::VectorFst<Arc>> cfst3_;
};

TEST_F(CountTest, PathCountTest) {
  fst::VectorFst<Arc> cfst2(*cfst2_);
  Counter<Arc> counter(0, kAlgoDelta, &cfst2);
  counter.Count(*cfst1_);
  counter.Finalize();
  ASSERT_TRUE(IsCanonical(cfst2, 0));
  ASSERT_TRUE(IsConservative(cfst2));
  ASSERT_TRUE(fst::Equal(*cfst3_, cfst2));
}

}  // namespace sfst

int main(int argc, char** argv) {
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
