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

// Unit test for CDRewriteCompile.

#include "opengrm/operators/cdrewrite.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/invert.h"
#include "openfst/lib/project.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/lib/verify.h"
#include "openfst/script/arc-class.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/verify.h"
#include "openfst/script/weight-class.h"
#include "opengrm/operators/cdrewritescript.h"
#include "opengrm/operators/crossscript.h"
#include "opengrm/operators/optimize.h"
#include "opengrm/operators/optimizescript.h"

namespace fst {
namespace {

using Arc = StdArc;

class CDRewriteCompileTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string data_dir = fst::JoinPath(
        std::string("."), "opengrm/operators/testdata");
    a_fst_.reset(VectorFst<Arc>::Read(fst::JoinPath(data_dir, "a.fst")));
    b_fst_.reset(VectorFst<Arc>::Read(fst::JoinPath(data_dir, "b.fst")));
    bXa_fst_.reset(VectorFst<Arc>::Read(fst::JoinPath(data_dir, "bXa.fst")));
    l2r_fst_.reset(VectorFst<Arc>::Read(fst::JoinPath(data_dir, "l2r.fst")));
    l2r_opt_fst_.reset(
        VectorFst<Arc>::Read(fst::JoinPath(data_dir, "l2r.opt.fst")));
    r2l_fst_.reset(VectorFst<Arc>::Read(fst::JoinPath(data_dir, "r2l.fst")));
    r2l_opt_fst_.reset(
        VectorFst<Arc>::Read(fst::JoinPath(data_dir, "r2l.opt.fst")));
    s_fst_.reset(VectorFst<Arc>::Read(fst::JoinPath(data_dir, "s.fst")));
    s_opt_fst_.reset(
        VectorFst<Arc>::Read(fst::JoinPath(data_dir, "s.opt.fst")));
    mapping_fst_.reset(
        VectorFst<Arc>::Read(fst::JoinPath(data_dir, "mapping.fst")));
    a_bos_fst_.reset(
        VectorFst<Arc>::Read(fst::JoinPath(data_dir, "a_bos.fst")));
    b_eos_fst_.reset(
        VectorFst<Arc>::Read(fst::JoinPath(data_dir, "b_eos.fst")));
    boundary_test_fst_.reset(
        VectorFst<Arc>::Read(fst::JoinPath(data_dir, "boundary_test.fst")));
  }

  std::unique_ptr<VectorFst<Arc>> a_fst_;
  std::unique_ptr<VectorFst<Arc>> b_fst_;
  std::unique_ptr<VectorFst<Arc>> a_bos_fst_;
  std::unique_ptr<VectorFst<Arc>> b_eos_fst_;
  std::unique_ptr<VectorFst<Arc>> bXa_fst_;
  std::unique_ptr<VectorFst<Arc>> boundary_test_fst_;
  std::unique_ptr<VectorFst<Arc>> l2r_fst_;
  std::unique_ptr<VectorFst<Arc>> l2r_opt_fst_;
  std::unique_ptr<VectorFst<Arc>> r2l_fst_;
  std::unique_ptr<VectorFst<Arc>> r2l_opt_fst_;
  std::unique_ptr<VectorFst<Arc>> s_fst_;
  std::unique_ptr<VectorFst<Arc>> s_opt_fst_;
  std::unique_ptr<VectorFst<Arc>> mapping_fst_;
};

TEST_F(CDRewriteCompileTest, CDRewriteCompile) {
  // Empty container FST.
  VectorFst<Arc> nfst;

  // Simple alphabet FST.
  VectorFst<Arc> sigma;
  sigma.SetStart(sigma.AddState());
  sigma.SetFinal(sigma.Start());
  sigma.AddArc(sigma.Start(), Arc(1, 1, sigma.Start()));
  sigma.AddArc(sigma.Start(), Arc(2, 2, sigma.Start()));
  sigma.AddArc(sigma.Start(), Arc(3, 3, sigma.Start()));

  // UTF-8-like alphabet FST.
  VectorFst<Arc> sigma2(ProjectFst<Arc>(*mapping_fst_, ProjectType::OUTPUT));

  // Rewrites a_fst_ and b_fst_ using this more complex alphabet.
  VectorFst<Arc> a2_fst(ComposeFst<Arc>(*a_fst_, *mapping_fst_));
  Project(&a2_fst, ProjectType::OUTPUT);
  VectorFst<Arc> b2_fst(ComposeFst<Arc>(*b_fst_, *mapping_fst_));
  Project(&b2_fst, ProjectType::OUTPUT);

  std::vector<CDRewriteDirection> directions;
  std::vector<CDRewriteMode> modes;
  std::vector<VectorFst<Arc>*> transducers;
  // Left-to-right obligatory
  directions.push_back(CDRewriteDirection::LEFT_TO_RIGHT);
  modes.push_back(CDRewriteMode::OBLIGATORY);
  transducers.push_back(l2r_fst_.get());
  // Left-to-right optional.
  directions.push_back(CDRewriteDirection::LEFT_TO_RIGHT);
  modes.push_back(CDRewriteMode::OPTIONAL);
  transducers.push_back(l2r_opt_fst_.get());
  // Right-to-left obligatory.
  directions.push_back(CDRewriteDirection::RIGHT_TO_LEFT);
  modes.push_back(CDRewriteMode::OBLIGATORY);
  transducers.push_back(r2l_fst_.get());
  // Right-to-left optional.
  directions.push_back(CDRewriteDirection::RIGHT_TO_LEFT);
  modes.push_back(CDRewriteMode::OPTIONAL);
  transducers.push_back(r2l_opt_fst_.get());
  // Simultaneous obligatory.
  directions.push_back(CDRewriteDirection::SIMULTANEOUS);
  modes.push_back(CDRewriteMode::OBLIGATORY);
  transducers.push_back(s_fst_.get());
  // Simultaneous optional.
  directions.push_back(CDRewriteDirection::SIMULTANEOUS);
  modes.push_back(CDRewriteMode::OPTIONAL);
  transducers.push_back(s_opt_fst_.get());

  VectorFst<Arc> tfst;
  VectorFst<Arc> t2fst;

  for (size_t i = 0; i < transducers.size(); ++i) {
    const auto& transducer = *transducers[i];
    const auto direction = directions[i];
    const auto mode = modes[i];

    // Tests behaviour on empty FSTs.
    CDRewriteCompile(nfst, nfst, nfst, nfst, sigma, &tfst, direction, mode,
                     false);
    ASSERT_TRUE(Verify(tfst));
    EXPECT_TRUE(Equal(tfst, sigma));

    // Tests using simple alphabet sigma.

    CDRewriteCompile(*b_fst_, *a_fst_, *a_fst_, *b_fst_, sigma, &tfst,
                     direction, mode, false);
    Optimize(&tfst);
    ASSERT_TRUE(Verify(tfst));
    EXPECT_TRUE(Equal(tfst, transducer));

    CDRewriteCompile(*bXa_fst_, *a_fst_, *b_fst_, sigma, &tfst, direction,
                     mode);
    Optimize(&tfst);
    ASSERT_TRUE(Verify(tfst));
    EXPECT_TRUE(Equal(tfst, transducer));

    // Tests using more complex alphabet sigma2.
    CDRewriteCompile(b2_fst, a2_fst, a2_fst, b2_fst, sigma2, &t2fst, direction,
                     mode, false);
    tfst = ComposeFst<Arc>(ComposeFst<Arc>(*mapping_fst_, t2fst),
                           InvertFst<Arc>(*mapping_fst_));
    Optimize(&tfst);
    ASSERT_TRUE(Verify(tfst));
    EXPECT_TRUE(Equal(tfst, transducer));
  }
}

TEST_F(CDRewriteCompileTest, NullContext) {
  // If for whatever reason one of lambda or rho is null, the context-dependent
  // rewrite should just be a no-op.

  // Empty container FST.
  VectorFst<Arc> nfst;

  // Simple alphabet FST.
  VectorFst<Arc> sigma;
  sigma.SetStart(sigma.AddState());
  sigma.SetFinal(sigma.Start());
  sigma.AddArc(sigma.Start(), Arc(1, 1, sigma.Start()));
  sigma.AddArc(sigma.Start(), Arc(2, 2, sigma.Start()));
  sigma.AddArc(sigma.Start(), Arc(3, 3, sigma.Start()));

  // FST accepting epsilon.
  VectorFst<Arc> epsilon;
  epsilon.SetStart(epsilon.AddState());
  epsilon.SetFinal(epsilon.Start());

  // Whatever direction and mode, we should get sigma-equivalent back.
  for (CDRewriteDirection dir :
       {CDRewriteDirection::LEFT_TO_RIGHT, CDRewriteDirection::RIGHT_TO_LEFT,
        CDRewriteDirection::SIMULTANEOUS}) {
    for (CDRewriteMode mode :
         {CDRewriteMode::OBLIGATORY, CDRewriteMode::OPTIONAL}) {
      VectorFst<Arc> tfst;
      CDRewriteCompile(*bXa_fst_, nfst, nfst, sigma, &tfst, dir, mode);
      EXPECT_TRUE(Equal(tfst, sigma))
          << "lambda: null rho: null dir: " << static_cast<int>(dir)
          << " mode: " << static_cast<int>(mode);
      CDRewriteCompile(*bXa_fst_, epsilon, nfst, sigma, &tfst, dir, mode);
      EXPECT_TRUE(Equal(tfst, sigma))
          << "lambda: epsilon rho: null dir: " << static_cast<int>(dir)
          << " mode: " << static_cast<int>(mode);
      CDRewriteCompile(*bXa_fst_, nfst, epsilon, sigma, &tfst, dir, mode);
      EXPECT_TRUE(Equal(tfst, sigma))
          << "lambda: null rho: epsilon dir: " << static_cast<int>(dir)
          << " mode: " << static_cast<int>(mode);
    }
  }
}

TEST_F(CDRewriteCompileTest, BoundaryMarkerTest) {
  VectorFst<Arc> sigma;
  sigma.SetStart(sigma.AddState());
  sigma.EmplaceArc(sigma.Start(), 1, 1, sigma.Start());
  sigma.EmplaceArc(sigma.Start(), 2, 2, sigma.Start());
  sigma.SetFinal(sigma.Start());
  VectorFst<Arc> tfst;
  CDRewriteCompile(*a_fst_, *b_fst_, *a_bos_fst_, *b_eos_fst_, sigma, &tfst,
                   CDRewriteDirection::LEFT_TO_RIGHT, CDRewriteMode::OBLIGATORY,
                   20, 21);
  Optimize(&tfst);
  ASSERT_TRUE(Verify(tfst));
  EXPECT_TRUE(Equal(tfst, *boundary_test_fst_));
}

// The same as the previous but with the scripting API.
TEST_F(CDRewriteCompileTest, CDRewriteScript) {
  namespace s = fst::script;

  s::VectorFstClass a_fst(*a_fst_);
  s::VectorFstClass b_fst(*b_fst_);
  s::VectorFstClass a_bos_fst(*a_bos_fst_);
  s::VectorFstClass b_eos_fst(*b_eos_fst_);
  s::VectorFstClass sigma("standard");
  sigma.SetStart(sigma.AddState());
  const auto one = s::WeightClass::One(sigma.WeightType());
  sigma.AddArc(sigma.Start(), s::ArcClass(1, 1, one, sigma.Start()));
  sigma.AddArc(sigma.Start(), s::ArcClass(2, 2, one, sigma.Start()));
  sigma.SetFinal(sigma.Start(), one);
  s::VectorFstClass c_fst(sigma.ArcType());
  s::Cross(a_fst, b_fst, &c_fst);
  s::VectorFstClass tfst(sigma.ArcType());
  s::CDRewriteCompile(c_fst, a_bos_fst, b_eos_fst, sigma, &tfst,
                      CDRewriteDirection::LEFT_TO_RIGHT,
                      CDRewriteMode::OBLIGATORY, 20, 21);
  s::Optimize(&tfst);
  ASSERT_TRUE(s::Verify(tfst));
  s::VectorFstClass boundary_test_fst(*boundary_test_fst_);
  EXPECT_TRUE(s::Equal(tfst, boundary_test_fst));
}

}  // namespace
}  // namespace fst
