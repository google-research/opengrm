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

#include "opengrm/rewrite/base_rule_cascade.h"

#include <string>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/types/span.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/equivalent.h"
#include "openfst/lib/string.h"
#include "openfst/lib/symbol-table.h"
#include "opengrm/operators/optimize.h"

namespace rewrite {
namespace {

using ::testing::ElementsAre;
using ::testing::UnorderedElementsAre;

class TestRuleCascade : public BaseRuleCascade<::fst::StdArc> {
 public:
  TestRuleCascade();

  bool Rewrite(const Transducer& input,
               MutableTransducer* lattice) const final {
    ::fst::Compose(input, rule_, lattice);
    return lattice->NumStates();
  }

  const ::fst::SymbolTable* GeneratedSymbols() const final {
    return &gen_syms_;
  }

 private:
  void AddPath(absl::Span<const std::pair<Label, Label>> labels,
               float final_weight) {
    auto state = rule_.Start();
    for (auto i : labels) {
      auto next_state = rule_.AddState();
      rule_.AddArc(state, {i.first, i.second, 0, next_state});
      state = next_state;
    }
    rule_.SetFinal(state, fst::StdArc::Weight(final_weight));
  }

  MutableTransducer rule_;
  ::fst::SymbolTable gen_syms_;
};

TestRuleCascade::TestRuleCascade() {
  gen_syms_.AddSymbol("<epsilon>", 0);

  rule_.SetStart(rule_.AddState());
  // "two" : "2" <0>
  AddPath({{'t', 0}, {'w', 0}, {'o', '2'}}, 0);
  // "two" : "ii" <1>
  AddPath({{'t', 'i'}, {'w', 'i'}, {'o', 0}}, 1);
  // "tie" : "tie" <0>
  AddPath({{'t', 't'}, {'i', 'i'}, {'e', 'e'}}, 0);
  // "tie" : "Tie" <0>
  AddPath({{'t', 'T'}, {'i', 'i'}, {'e', 'e'}}, 0);
  // "tie" : "TIE" <1>
  AddPath({{'t', 'T'}, {'i', 'I'}, {'e', 'E'}}, 1);
  // "dup" : "2" <0>
  AddPath({{'d', 0}, {'u', 0}, {'p', '2'}}, 0);
  // "dup" : "ii" <1>
  AddPath({{'d', 'i'}, {'u', 'i'}, {'p', 0}}, 1);
  // "dup" : "2" <2>
  AddPath({{'d', 0}, {'u', 0}, {'p', '2'}}, 2);

  ::fst::ArcSort(&rule_, ::fst::ILabelCompare<MutableTransducer::Arc>());
}

TEST(BaseRuleCascadeTest, TopRewrite) {
  TestRuleCascade cascade;
  TestRuleCascade::MutableTransducer output_fst;
  TestRuleCascade::MutableTransducer expect_fst;
  std::vector<TestRuleCascade::Label> output_labels;
  std::string output_string;
  std::string debug_string;

  EXPECT_FALSE(cascade.TopRewrite("fail", &output_fst));
  EXPECT_FALSE(cascade.TopRewrite("fail", &output_labels));
  EXPECT_FALSE(cascade.TopRewrite("fail", &output_string));
  EXPECT_FALSE(cascade.TopRewrite("fail", &output_string, &debug_string));

  EXPECT_TRUE(cascade.TopRewrite("two", &output_fst));
  ::fst::Optimize(&output_fst);
  const ::fst::StringCompiler<::fst::StdArc> compiler(::fst::TokenType::BYTE);
  compiler("2", &expect_fst);
  EXPECT_TRUE(::fst::Equivalent(output_fst, expect_fst));

  EXPECT_TRUE(cascade.TopRewrite("two", &output_labels));
  EXPECT_THAT(output_labels, ElementsAre('2'));

  EXPECT_TRUE(cascade.TopRewrite("two", &output_string));
  EXPECT_EQ("2", output_string);

  EXPECT_TRUE(cascade.TopRewrite("two", &output_string, &debug_string));
  EXPECT_EQ("2", output_string);
  EXPECT_EQ("2", debug_string);
}

TEST(BaseRuleCascadeTest, OneTopRewrite) {
  TestRuleCascade cascade;
  TestRuleCascade::MutableTransducer output_fst;
  TestRuleCascade::MutableTransducer expect_fst;
  std::vector<TestRuleCascade::Label> output_labels;
  std::string output_string;
  std::string debug_string;

  EXPECT_FALSE(cascade.OneTopRewrite("fail", &output_fst));
  EXPECT_FALSE(cascade.OneTopRewrite("fail", &output_labels));
  EXPECT_FALSE(cascade.OneTopRewrite("fail", &output_string));
  EXPECT_FALSE(cascade.OneTopRewrite("fail", &output_string, &debug_string));

  EXPECT_FALSE(cascade.OneTopRewrite("tie", &output_fst));
  output_fst.Write("/tmp/debug.fst");
  EXPECT_FALSE(cascade.OneTopRewrite("tie", &output_labels));
  EXPECT_FALSE(cascade.OneTopRewrite("tie", &output_string));
  EXPECT_FALSE(cascade.OneTopRewrite("tie", &output_string, &debug_string));

  EXPECT_TRUE(cascade.OneTopRewrite("two", &output_fst));
  EXPECT_TRUE(cascade.TopRewrite("two", &expect_fst));
  ::fst::Optimize(&output_fst);
  ::fst::Optimize(&expect_fst);
  EXPECT_TRUE(::fst::Equivalent(output_fst, expect_fst));

  EXPECT_TRUE(cascade.OneTopRewrite("two", &output_labels));
  EXPECT_THAT(output_labels, ElementsAre('2'));

  EXPECT_TRUE(cascade.OneTopRewrite("two", &output_string));
  EXPECT_EQ("2", output_string);

  EXPECT_TRUE(cascade.OneTopRewrite("two", &output_string, &debug_string));
  EXPECT_EQ("2", output_string);
  EXPECT_EQ("2", debug_string);
}

TEST(BaseRuleCascadeTest, NTopRewrites) {
  TestRuleCascade cascade;
  TestRuleCascade::MutableTransducer output_fst;
  TestRuleCascade::MutableTransducer expect_fst;
  std::vector<std::vector<TestRuleCascade::Label>> output_labels;
  std::vector<std::string> output_strings;
  std::vector<std::string> debug_strings;

  EXPECT_FALSE(cascade.TopRewrites("fail", 2, &output_fst));
  EXPECT_FALSE(cascade.TopRewrites("fail", 2, &output_labels));
  EXPECT_FALSE(cascade.TopRewrites("fail", 2, &output_strings));
  EXPECT_FALSE(cascade.TopRewrites("fail", 2, &output_strings, &debug_strings));

  EXPECT_TRUE(cascade.TopRewrites("dup", 1, &output_fst));
  EXPECT_TRUE(cascade.TopRewrite("dup", &expect_fst));
  ::fst::Optimize(&output_fst);
  ::fst::Optimize(&expect_fst);
  EXPECT_TRUE(::fst::Equivalent(output_fst, expect_fst));

  EXPECT_TRUE(cascade.TopRewrites("dup", 3, &output_labels));
  EXPECT_THAT(output_labels, ElementsAre(ElementsAre(0, 0, '2', 0),
                                         ElementsAre(0, 0, 'i', 'i', 0)));

  EXPECT_TRUE(cascade.TopRewrites("dup", 3, &output_strings));
  EXPECT_THAT(output_strings, ElementsAre("2", "ii"));

  EXPECT_TRUE(cascade.TopRewrites("dup", 3, &output_strings, &debug_strings));
  EXPECT_THAT(output_strings, ElementsAre("2", "ii"));
  EXPECT_THAT(debug_strings,
              ElementsAre("[<epsilon>][<epsilon>]2[<epsilon>]",
                          "[<epsilon>][<epsilon>]ii[<epsilon>]"));
}

TEST(BaseRuleCascadeTest, TopRewrites) {
  TestRuleCascade cascade;
  TestRuleCascade::MutableTransducer output_fst, expect_fst;
  std::vector<std::vector<TestRuleCascade::Label>> output_labels;
  std::vector<std::string> output_string, debug_string;

  EXPECT_FALSE(cascade.TopRewrites("fail", &output_fst));
  EXPECT_FALSE(cascade.TopRewrites("fail", &output_labels));
  EXPECT_FALSE(cascade.TopRewrites("fail", &output_string));
  EXPECT_FALSE(cascade.TopRewrites("fail", &output_string, &debug_string));

  // Unique top rewrites.
  EXPECT_TRUE(cascade.TopRewrites("two", &output_fst));
  EXPECT_TRUE(cascade.TopRewrite("two", &expect_fst));
  ::fst::Optimize(&output_fst);
  ::fst::Optimize(&expect_fst);
  EXPECT_TRUE(::fst::Equivalent(output_fst, expect_fst));

  EXPECT_TRUE(cascade.TopRewrites("two", &output_labels));
  EXPECT_THAT(output_labels, ElementsAre(ElementsAre('2')));

  EXPECT_TRUE(cascade.TopRewrites("two", &output_string));
  EXPECT_THAT(output_string, ElementsAre("2"));

  EXPECT_TRUE(cascade.TopRewrites("two", &output_string, &debug_string));
  EXPECT_THAT(output_string, ElementsAre("2"));
  EXPECT_THAT(debug_string, ElementsAre("2"));

  // Multiple top rewrites.
  EXPECT_TRUE(cascade.TopRewrites("tie", &output_fst));
  EXPECT_TRUE(cascade.TopRewrites("tie", 2, &expect_fst));
  ::fst::Optimize(&output_fst);
  ::fst::Optimize(&expect_fst);
  EXPECT_TRUE(::fst::Equivalent(output_fst, expect_fst));

  EXPECT_TRUE(cascade.TopRewrites("tie", &output_labels));
  EXPECT_THAT(output_labels, UnorderedElementsAre(ElementsAre('t', 'i', 'e'),
                                                  ElementsAre('T', 'i', 'e')));

  EXPECT_TRUE(cascade.TopRewrites("tie", &output_string));
  EXPECT_THAT(output_string, UnorderedElementsAre("tie", "Tie"));

  EXPECT_TRUE(cascade.TopRewrites("tie", &output_string, &debug_string));
  EXPECT_THAT(output_string, UnorderedElementsAre("tie", "Tie"));
  EXPECT_THAT(debug_string, UnorderedElementsAre("tie", "Tie"));
}

TEST(BaseRuleCascadeTest, Rewrites) {
  TestRuleCascade cascade;
  TestRuleCascade::MutableTransducer output_fst;
  TestRuleCascade::MutableTransducer expect_fst;
  std::vector<std::vector<TestRuleCascade::Label>> output_labels;
  std::vector<std::string> output_strings;
  std::vector<std::string> debug_strings;

  EXPECT_FALSE(cascade.Rewrites("fail", &output_fst));
  EXPECT_FALSE(cascade.Rewrites("fail", &output_labels));
  EXPECT_FALSE(cascade.Rewrites("fail", &output_strings));
  EXPECT_FALSE(cascade.Rewrites("fail", &output_strings, &debug_strings));

  EXPECT_TRUE(cascade.Rewrites("two", &output_fst));
  EXPECT_TRUE(cascade.TopRewrites("two", 2, &expect_fst));
  ::fst::Optimize(&output_fst);
  ::fst::Optimize(&expect_fst);
  EXPECT_TRUE(::fst::Equivalent(output_fst, expect_fst));

  EXPECT_TRUE(cascade.Rewrites("two", &output_labels));
  EXPECT_THAT(output_labels,
              ElementsAre(ElementsAre('2'), ElementsAre('i', 'i')));

  EXPECT_TRUE(cascade.Rewrites("two", &output_strings));
  EXPECT_THAT(output_strings, ElementsAre("2", "ii"));

  EXPECT_TRUE(cascade.Rewrites("two", &output_strings, &debug_strings));
  EXPECT_THAT(output_strings, ElementsAre("2", "ii"));
  EXPECT_THAT(debug_strings, ElementsAre("2", "ii"));
}

TEST(BaseRuleCascadeTest, Matches) {
  TestRuleCascade cascade;
  EXPECT_FALSE(cascade.Matches("two", "two"));
  EXPECT_TRUE(cascade.Matches("two", "2"));
  EXPECT_TRUE(cascade.Matches("two", "ii"));
}

}  // namespace
}  // namespace rewrite
