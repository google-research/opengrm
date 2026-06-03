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

// Tests for Bayesian merging of NGramModels.

#include "opengrm/ngram/ngram-bayes-model-merge.h"

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "opengrm/ngram/ngram-model.h"

namespace ngram {

using ::fst::Equal;
using ::fst::StdArc;
using ::fst::StdFst;
using ::fst::StdVectorFst;
using ::fst::Verify;

class NGramMergeMethodsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string path = fst::JoinPath(
        std::string("."), "opengrm/ngram/testdata");
    lm1_fst_.reset(StdVectorFst::Read(fst::JoinPath(path, "earnest1.mod")));
    lm2_fst_.reset(StdVectorFst::Read(fst::JoinPath(path, "earnest2.mod")));
    lm3_fst_.reset(StdVectorFst::Read(fst::JoinPath(path, "earnest.bayes")));
  }

  std::unique_ptr<StdFst> lm1_fst_;
  std::unique_ptr<StdFst> lm2_fst_;
  std::unique_ptr<StdFst> lm3_fst_;
};

// Checks state probs are correctly computed.
TEST_F(NGramMergeMethodsTest, StateProbsTest) {
  NGramModel<StdArc> ngrammodel(*lm1_fst_, 0, ngram::kNormEps, true);
  std::vector<double> probs;
  ngrammodel.CalculateStateProbs(&probs);
  for (int st = 0; st < ngrammodel.NumStates(); ++st) {
    const std::vector<int>& ngram = ngrammodel.StateNGram(st);
    double cost = ngrammodel.ScalarValue(ngrammodel.GetNGramCost(ngram));
    EXPECT_FLOAT_EQ(cost, -log(probs[st]));
  }
}

// Checks new bayes merging.
TEST_F(NGramMergeMethodsTest, NGramBayesMergeTest) {
  StdVectorFst merged_fst(*lm1_fst_);
  NGramBayesModelMerge ngrammerge(&merged_fst);
  ngrammerge.MergeNGramModels(*lm2_fst_, .3, .7);
  ngrammerge.SortStates();
  ngrammerge.InitModel();
  Verify(merged_fst);
  ASSERT_TRUE(Equal(merged_fst, *lm3_fst_));
}

}  // namespace ngram
