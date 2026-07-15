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

#include "opengrm/thrax/walker/util/function/cdrewrite.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc-map.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/isomorphic.h"
#include "openfst/lib/project.h"
#include "openfst/lib/rmepsilon.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

template <typename Arc>
class CDRewriteTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  void SetUp() override {
    // tau: transformation transducer (a -> b).
    auto tau = std::make_unique<MutableTransducer>();
    {
      auto p = tau->AddState();
      tau->SetStart(p);
      auto q = tau->AddState();
      tau->EmplaceArc(p, 'a', 'b', q);
      tau->SetFinal(q);
    }

    // lambda: unweighted left context acceptor ("c").
    auto lambda = std::make_unique<MutableTransducer>();
    MakeStringFst("c", lambda.get());

    // rho: unweighted right context acceptor ("d").
    auto rho = std::make_unique<MutableTransducer>();
    MakeStringFst("d", rho.get());

    // sigma: unweighted alphabet closure acceptor (sigma* = (a|b|c|d)*).
    auto sigma = std::make_unique<MutableTransducer>();
    {
      auto p = sigma->AddState();
      sigma->SetStart(p);
      sigma->SetFinal(p);
      sigma->EmplaceArc(p, 'a', 'a', p);
      sigma->EmplaceArc(p, 'b', 'b', p);
      sigma->EmplaceArc(p, 'c', 'c', p);
      sigma->EmplaceArc(p, 'd', 'd', p);
    }

    auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(4);
    (*args)[0] = std::make_unique<DataType>(std::move(tau));
    (*args)[1] = std::make_unique<DataType>(std::move(lambda));
    (*args)[2] = std::make_unique<DataType>(std::move(rho));
    (*args)[3] = std::make_unique<DataType>(std::move(sigma));

    rule_data_ = func_.Run(std::move(args));
    ASSERT_NE(rule_data_, nullptr);
    rule_ = *rule_data_->template get<Transducer*>();
  }

  static void MakeStringFst(absl::string_view s, MutableTransducer* fst) {
    fst->DeleteStates();
    auto current = fst->AddState();
    fst->SetStart(current);
    for (char c : s) {
      auto next = fst->AddState();
      fst->EmplaceArc(current, c, c, next);
      current = next;
    }
    fst->SetFinal(current);
  }

  void VerifyRewrite(absl::string_view input_str,
                     absl::string_view expected_str) {
    ASSERT_NE(rule_, nullptr);
    MutableTransducer input;
    MakeStringFst(input_str, &input);

    MutableTransducer output;
    ::fst::Compose(input, *rule_, &output);

    MutableTransducer clean_output = output;
    ::fst::Project(&clean_output, ::fst::ProjectType::OUTPUT);
    ::fst::RmEpsilon(&clean_output);
    ::fst::ArcMap(&clean_output, ::fst::RmWeightMapper<Arc>());

    MutableTransducer expected;
    MakeStringFst(expected_str, &expected);

    EXPECT_TRUE(::fst::Isomorphic(clean_output, expected))
        << "Expected input \"" << input_str << "\" to rewrite to \""
        << expected_str << "\"";
  }

  CDRewrite<Arc> func_;
  std::unique_ptr<DataType> rule_data_;
  const Transducer* rule_ = nullptr;
};

using ArcTypes = ::testing::Types<::fst::StdArc, ::fst::LogArc>;
TYPED_TEST_SUITE(CDRewriteTest, ArcTypes, );

// Classical context-dependent rewrite rules take the form:
//   phi -> psi / lambda __ rho
// where target sequence phi is rewritten to replacement psi whenever it is
// preceded by left context lambda and followed by right context rho, operating
// over alphabet closure sigma*.
//
// In OpenGrm/Thrax, rather than passing separate phi and psi acceptors,
// CDRewrite takes four components mapped directly to canonical FST notation:
//   tau:    rewrite transducer representing phi X psi transformation
//   lambda: unweighted left context acceptor ("L" -> lambda)
//   rho:    unweighted right context acceptor ("R" -> rho)
//   sigma:  unweighted alphabet closure acceptor (sigma*)

TYPED_TEST(CDRewriteTest, CompilesRewriteRuleSuccessfully) {
  EXPECT_NE(this->rule_, nullptr);
}

TYPED_TEST(CDRewriteTest, AppliesRewriteWhenFullContextMatches) {
  // Case 1: Context matches "cad" -> "cbd"
  this->VerifyRewrite("cad", "cbd");
}

TYPED_TEST(CDRewriteTest, IgnoresWhenRightContextMissing) {
  // Case 2: Context doesn't match "ca" -> "ca" (missing right context 'd')
  this->VerifyRewrite("ca", "ca");
}

TYPED_TEST(CDRewriteTest, IgnoresWhenLeftContextMissing) {
  // Left context missing ("ad" -> "ad")
  this->VerifyRewrite("ad", "ad");
}

TYPED_TEST(CDRewriteTest, AppliesMultipleMatchesInSequence) {
  // Multiple occurrences inside one string ("cadcad" -> "cbdcbd")
  this->VerifyRewrite("cadcad", "cbdcbd");
}

}  // namespace function
}  // namespace thrax
