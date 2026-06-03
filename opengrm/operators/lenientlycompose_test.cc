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

#include "opengrm/operators/lenientlycompose.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/project.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/project.h"
#include "openfst/script/verify.h"
#include "opengrm/operators/lenientlycomposescript.h"
#include "opengrm/operators/optimize.h"
#include "opengrm/operators/optimizescript.h"

namespace fst {
namespace {

using Arc = StdArc;

// Example based on:
//
// L. Karttunen. 1998. The proper treatment of Optimality Theory in
// computational phonology. In Proc. FSMNLP, pages 1-12.
//
// The Q relation is: A -> X, B -> Y.
// The R relation is: B -> Z, C-> W.
// The priority union of Q and R, P, is the relation: A -> X, B -> Y, C -> X.

class LenientlyComposeTest : public ::testing::Test {
 protected:
  void SetUp() final {
    const std::string testdir = fst::JoinPath(
        std::string("."), "opengrm/operators/testdata");

    const std::string sigma_name = fst::JoinPath(testdir, "sigma.fst");
    const std::string eggs_name = fst::JoinPath(testdir, "eggs.fst");
    const std::string noise_name = fst::JoinPath(testdir, "noise.fst");
    const std::string spam_name = fst::JoinPath(testdir, "spam.fst");
    const std::string spam2eggs_name = fst::JoinPath(testdir, "spam2eggs.fst");

    sigma_.reset(VectorFst<Arc>::Read(sigma_name));

    eggs_.reset(VectorFst<Arc>::Read(eggs_name));
    noise_.reset(VectorFst<Arc>::Read(noise_name));
    spam_.reset(VectorFst<Arc>::Read(spam_name));
    spam2eggs_.reset(VectorFst<Arc>::Read(spam2eggs_name));
  }

  std::unique_ptr<VectorFst<Arc>> sigma_;

  std::unique_ptr<VectorFst<Arc>> eggs_;
  std::unique_ptr<VectorFst<Arc>> noise_;
  std::unique_ptr<VectorFst<Arc>> spam_;
  std::unique_ptr<VectorFst<Arc>> spam2eggs_;
};

TEST_F(LenientlyComposeTest, LenientlyCompose) {
  VectorFst<Arc> rfst;
  // Applies spam2eggs to input, successfully.
  LenientlyCompose(*spam_, *spam2eggs_, *sigma_, &rfst);
  Project(&rfst, ProjectType::OUTPUT);
  Optimize(&rfst);
  EXPECT_TRUE(Verify(rfst));
  EXPECT_TRUE(Equal(rfst, *eggs_));
  // Fails to apply, so result of application is the same as the input.
  LenientlyCompose(*noise_, *spam2eggs_, *sigma_, &rfst);
  Project(&rfst, ProjectType::OUTPUT);
  Optimize(&rfst);
  EXPECT_TRUE(Verify(rfst));
  EXPECT_TRUE(Equal(rfst, *noise_));
}

TEST_F(LenientlyComposeTest, FstClassLenientlyCompose) {
  namespace s = fst::script;
  s::VectorFstClass sigma(*sigma_);
  s::VectorFstClass eggs(*eggs_);
  s::VectorFstClass noise(*noise_);
  s::VectorFstClass spam(*spam_);
  s::VectorFstClass spam2eggs(*spam2eggs_);
  s::VectorFstClass rfst(sigma.ArcType());
  // Applies spam2eggs to input, successfully.
  s::LenientlyCompose(spam, spam2eggs, sigma, &rfst);
  s::Project(&rfst, ProjectType::OUTPUT);
  s::Optimize(&rfst);
  EXPECT_TRUE(s::Verify(rfst));
  EXPECT_TRUE(s::Equal(rfst, eggs));
  // Fails to apply, so result of application is the same as the input.
  s::LenientlyCompose(noise, spam2eggs, sigma, &rfst);
  s::Project(&rfst, ProjectType::OUTPUT);
  s::Optimize(&rfst);
  EXPECT_TRUE(s::Verify(rfst));
  EXPECT_TRUE(s::Equal(rfst, noise));
}

}  // namespace
}  // namespace fst
