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

#include "opengrm/operators/cross.h"

#include <memory>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/string.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/weight-class.h"
#include "opengrm/operators/crossscript.h"
#include "opengrm/string/stringcompile.h"
#include "opengrm/string/stringcompilescript.h"

namespace fst {
namespace {

using Arc = StdArc;

class CrossTest : public ::testing::Test {
 protected:
  void SetUp() final {
    const std::string upper_name = fst::JoinPath(
        std::string("."),
        "opengrm/operators/testdata/upper.fst");
    const std::string lower_name = fst::JoinPath(
        std::string("."),
        "opengrm/operators/testdata/lower.fst");
    const std::string xprod_name = fst::JoinPath(
        std::string("."),
        "opengrm/operators/testdata/xprod.fst");

    s1_ = R"(Cheddar?")";
    s2_ = R"(I'm afraid we haven't got much call for it around these parts.)";

    lower_.reset(VectorFst<Arc>::Read(upper_name));
    upper_.reset(VectorFst<Arc>::Read(lower_name));
    xprod_.reset(VectorFst<Arc>::Read(xprod_name));
  }

  std::unique_ptr<VectorFst<Arc>> lower_;
  std::unique_ptr<VectorFst<Arc>> upper_;
  std::unique_ptr<VectorFst<Arc>> xprod_;

  const char* s1_;
  const char* s2_;
};

TEST_F(CrossTest, ArbitraryInput) {
  VectorFst<Arc> res;
  Cross(*lower_, *upper_, &res);
  EXPECT_TRUE(Equal(res, *xprod_));
}

TEST_F(CrossTest, FstClassArbitraryInput) {
  namespace s = fst::script;
  s::VectorFstClass lower(*lower_);
  s::VectorFstClass upper(*upper_);
  s::VectorFstClass res(lower.ArcType());
  Cross(lower, upper, &res);
  s::VectorFstClass xprod(*xprod_);
  EXPECT_TRUE(s::Equal(res, xprod));
}

TEST_F(CrossTest, StringInput) {
  VectorFst<Arc> s1_res;
  EXPECT_TRUE(StringCompile(s1_, &s1_res));
  VectorFst<Arc> s2_res;
  EXPECT_TRUE(StringCompile(s2_, &s2_res));
  VectorFst<Arc> res;
  Cross(s1_res, s2_res, &res);
  EXPECT_EQ(63, res.NumStates());
}

TEST_F(CrossTest, FstClassStringInput) {
  namespace s = fst::script;
  const std::string arc_type = "standard";
  const auto one = s::WeightClass::One("tropical");
  s::VectorFstClass s1_res(arc_type);
  EXPECT_TRUE(StringCompile(s1_, &s1_res, TokenType::BYTE, nullptr, one));
  s::VectorFstClass s2_res(arc_type);
  EXPECT_TRUE(StringCompile(s2_, &s2_res, TokenType::BYTE, nullptr, one));
  s::VectorFstClass res(arc_type);
  Cross(s1_res, s2_res, &res);
  EXPECT_EQ(63, res.NumStates());
}

}  // namespace
}  // namespace fst
