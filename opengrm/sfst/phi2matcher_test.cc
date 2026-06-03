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

// Unit tests FST matcher and composition filter that handle failure
// transitions on both components of a composition.

#include "opengrm/sfst/phi2matcher.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/matcher.h"
#include "openfst/lib/relabel.h"
#include "openfst/lib/test-properties.h"
#include "openfst/lib/vector-fst.h"

namespace sfst {

typedef fst::StdArc Arc;
typedef Arc::StateId StateId;
typedef Arc::Weight Weight;
typedef Arc::Label Label;

// Test class.
class Phi2MatcherTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string sfst1_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/phi1.fst");
    sfst1_.reset(fst::Fst<Arc>::Read(sfst1_name));

    const std::string sfst2_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/phi2.fst");
    sfst2_.reset(fst::Fst<Arc>::Read(sfst2_name));

    const std::string sfst3_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/phi3.fst");
    sfst3_.reset(fst::Fst<Arc>::Read(sfst3_name));

    const std::string sfst4_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/phi4.fst");
    sfst4_.reset(fst::Fst<Arc>::Read(sfst4_name));

    const std::string sfst5_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/phi5.fst");
    sfst5_.reset(fst::Fst<Arc>::Read(sfst5_name));

    const std::string sfst6_name =
        fst::JoinPath(std::string("."),
                       "opengrm/sfst/testdata/phi6.fst");
    sfst6_.reset(fst::Fst<Arc>::Read(sfst6_name));
  }

  std::unique_ptr<fst::Fst<Arc>> sfst1_;
  std::unique_ptr<fst::Fst<Arc>> sfst2_;
  std::unique_ptr<fst::Fst<Arc>> sfst3_;
  std::unique_ptr<fst::Fst<Arc>> sfst4_;
  std::unique_ptr<fst::Fst<Arc>> sfst5_;
  std::unique_ptr<fst::Fst<Arc>> sfst6_;
};

// Mini-bigram model test
TEST_F(Phi2MatcherTest, Phi2BiMatcher) {
  using PM = Phi2Matcher<fst::Matcher<fst::Fst<Arc>>>;
  using PF = Phi2Filter<PM>;
  fst::ComposeFstOptions<Arc, PM, PF> copts;

  copts.gc_limit = 0;
  copts.matcher1 = new PM(*sfst1_, fst::MATCH_OUTPUT, 0);
  copts.matcher2 = new PM(*sfst2_, fst::MATCH_INPUT, 0);
  fst::ComposeFst<Arc> cfst(*sfst1_, *sfst2_, copts);
  ASSERT_TRUE(fst::Equal(cfst, *sfst3_));
  ASSERT_TRUE(true);
}

// Mini-trigram model test
TEST_F(Phi2MatcherTest, Phi2TriMatcher) {
  using PM = Phi2Matcher<fst::Matcher<fst::Fst<Arc>>>;
  using PF = Phi2Filter<PM>;
  fst::ComposeFstOptions<Arc, PM, PF> copts;

  copts.gc_limit = 0;
  copts.matcher1 = new PM(*sfst4_, fst::MATCH_OUTPUT, 0);
  copts.matcher2 = new PM(*sfst5_, fst::MATCH_INPUT, 0);
  fst::ComposeFst<Arc> cfst(*sfst4_, *sfst5_, copts);
  ASSERT_TRUE(fst::Equal(cfst, *sfst6_));
  ASSERT_TRUE(true);
}

// Mini-trigram model test where phi_label is non-zero
TEST_F(Phi2MatcherTest, NonZeroPhiMatcher) {
  using PM = Phi2Matcher<fst::Matcher<fst::Fst<Arc>>>;
  using PF = Phi2Filter<PM>;
  fst::ComposeFstOptions<Arc, PM, PF> copts;
  const Label phi_label = -2;
  fst::VectorFst<Arc> sfst4(*sfst4_);
  fst::VectorFst<Arc> sfst5(*sfst5_);
  std::vector<std::pair<Label, Label>> pairs = {{0, phi_label}};
  std::vector<std::pair<Label, Label>> invpairs = {{phi_label, 0}};
  fst::Relabel(&sfst4, pairs, pairs);
  fst::Relabel(&sfst5, pairs, pairs);

  copts.gc_limit = 0;
  copts.matcher1 = new PM(sfst4, fst::MATCH_OUTPUT, phi_label);
  copts.matcher2 = new PM(sfst5, fst::MATCH_INPUT, phi_label);
  fst::ComposeFst<Arc> cfst(sfst4, sfst5, copts);
  fst::VectorFst<Arc> sfst6(cfst);
  fst::Relabel(&sfst6, invpairs, invpairs);
  ASSERT_TRUE(fst::Equal(sfst6, *sfst6_));
  ASSERT_TRUE(true);
}

}  // namespace sfst

int main(int argc, char** argv) {
  absl::SetFlag(&FLAGS_fst_verify_properties, true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
