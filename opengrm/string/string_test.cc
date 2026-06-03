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

#include "openfst/lib/string.h"

#include <memory>
#include <sstream>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/expanded-fst.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/verify.h"
#include "openfst/script/weight-class.h"
#include "opengrm/string/defaults.h"
#include "opengrm/string/stringcompile.h"
#include "opengrm/string/stringcompilescript.h"
#include "opengrm/string/stringprint.h"
#include "opengrm/string/stringprintscript.h"

namespace fst {
namespace {

using Arc = StdArc;
using Weight = Arc::Weight;

class StringTest : public ::testing::Test {
 protected:
  void SetUp() override {
    s1_ = R"(Hello, world!)";
    s2_ = R"([72][101][108][108][111][44][32][119][111][114][108][100][33])";

    s3_ = R"(año māl coöperation über résumé être očudit Pająk fracoð þæt )"
          R"(Straße açai ealneġ)";

    s4_ = R"(田中さんにあげて下さい 사회과학원 معاملة بولندا، الإطلاق عل إيو)";

    s5_ = R"([año māl coöperation über résumé être očudit Pająk fracoð þæt )"
          R"(Straße açai ealneġ])";
    s6_ = R"([año][māl][coöperation][über][résumé][être][očudit][Pająk])"
          R"([fracoð][þæt][Straße][açai][ealneġ])";

    s7_ = R"(Regional [año māl coöperation] être [þæt Straße 0141 97 0x61])";

    s8_ = R"(An \[expression [\[with escaped] brackets\])";
    s9_ = R"([BOS]from the mountains to the hills[EOS])";

    s10_ = R"(ending with a slash\)";
    s11_ = R"(\n\\n\r\\r\t\\t)";
    s12_ = R"(\a\\a\#\\#)";

    const std::string testdir = fst::JoinPath(
        std::string("."), "opengrm/string/testdata");
    const std::string s3_byte_name = fst::JoinPath(testdir, "s3_byte.fst");
    const std::string s3_utf8_name = fst::JoinPath(testdir, "s3_utf8.fst");
    const std::string s3_syms_name = fst::JoinPath(testdir, "s3_syms.fst");
    const std::string s4_byte_name = fst::JoinPath(testdir, "s4_byte.fst");
    const std::string s4_utf8_name = fst::JoinPath(testdir, "s4_utf8.fst");
    const std::string s7_byte_name = fst::JoinPath(testdir, "s7_byte.fst");
    const std::string s7_utf8_name = fst::JoinPath(testdir, "s7_utf8.fst");
    const std::string s8_byte_name = fst::JoinPath(testdir, "s8_byte.fst");
    const std::string s8_utf8_name = fst::JoinPath(testdir, "s8_byte.fst");
    const std::string weighted_name = fst::JoinPath(testdir, "weighted.fst");
    const std::string symbols_name = fst::JoinPath(testdir, "sym.map");

    s3_byte_.reset(VectorFst<Arc>::Read(s3_byte_name));
    s3_utf8_.reset(VectorFst<Arc>::Read(s3_utf8_name));
    s3_syms_.reset(VectorFst<Arc>::Read(s3_syms_name));
    s4_byte_.reset(VectorFst<Arc>::Read(s4_byte_name));
    s4_utf8_.reset(VectorFst<Arc>::Read(s4_utf8_name));
    s7_byte_.reset(VectorFst<Arc>::Read(s7_byte_name));
    s7_utf8_.reset(VectorFst<Arc>::Read(s7_utf8_name));
    s8_byte_.reset(VectorFst<Arc>::Read(s8_byte_name));
    s8_utf8_.reset(VectorFst<Arc>::Read(s8_utf8_name));
    weighted_.reset(VectorFst<Arc>::Read(weighted_name));
    symbols_.reset(SymbolTable::ReadText(symbols_name));
  }

  template <class FstOrFstClass>
  bool CheckFst(const FstOrFstClass& fst) const {
    constexpr auto kProps = kString | kAcyclic | kAcceptor;
    return Verify(fst) && (fst.Properties(kProps, true) == kProps);
  }

  std::string s1_;
  std::string s2_;
  std::string s3_;
  std::string s4_;
  std::string s5_;
  std::string s6_;
  std::string s7_;
  std::string s8_;
  std::string s9_;
  std::string s10_;
  std::string s11_;
  std::string s12_;

  std::unique_ptr<VectorFst<Arc>> s3_byte_;
  std::unique_ptr<VectorFst<Arc>> s3_utf8_;
  std::unique_ptr<VectorFst<Arc>> s3_syms_;
  std::unique_ptr<VectorFst<Arc>> s4_byte_;
  std::unique_ptr<VectorFst<Arc>> s4_utf8_;
  std::unique_ptr<VectorFst<Arc>> s7_byte_;
  std::unique_ptr<VectorFst<Arc>> s7_utf8_;
  std::unique_ptr<VectorFst<Arc>> s8_byte_;
  std::unique_ptr<VectorFst<Arc>> s8_utf8_;
  std::unique_ptr<VectorFst<Arc>> weighted_;
  std::unique_ptr<SymbolTable> symbols_;
};

TEST_F(StringTest, AsciiTest) {
  VectorFst<Arc> s1_res;
  std::string str;

  // Bytestring compilation of ASCII string.
  ASSERT_TRUE(StringCompile(s1_, &s1_res));
  ASSERT_TRUE(CheckFst(s1_res));
  ASSERT_TRUE(StringPrint(s1_res, &str));
  EXPECT_EQ(str, s1_);

  // Compilation from bracketed integers should produce the same result.
  VectorFst<Arc> s2_res;
  ASSERT_TRUE(StringCompile(s2_, &s2_res));
  ASSERT_TRUE(CheckFst(s2_res));
  ASSERT_TRUE(StringPrint(s2_res, &str));
  EXPECT_EQ(str, s1_);  // Compares against "canonical" form of string.

  ASSERT_TRUE(Equal(s1_res, s2_res));
}

TEST_F(StringTest, BracketedIntegerString) {
  std::stringstream sstrm;
  for (int i = 1; i < 300; ++i) sstrm << "[" << i << "]";
  VectorFst<Arc> res;
  ASSERT_TRUE(StringCompile(sstrm.str(), &res));
  ASSERT_TRUE(CheckFst(res));
  EXPECT_EQ(300, CountStates(res));
}

TEST_F(StringTest, UTF8Test) {
  VectorFst<Arc> res;
  std::string str;

  // Bytestring compilation of UTF-8 strings.
  ASSERT_TRUE(StringCompile(s3_, &res, TokenType::BYTE));
  ASSERT_TRUE(CheckFst(res));
  ASSERT_TRUE(Equal(*s3_byte_, res));
  ASSERT_TRUE(StringPrint(res, &str, TokenType::BYTE));
  EXPECT_EQ(str, s3_);

  ASSERT_TRUE(StringCompile(s4_, &res, TokenType::BYTE));
  ASSERT_TRUE(CheckFst(res));
  ASSERT_TRUE(Equal(*s4_byte_, res));
  ASSERT_TRUE(StringPrint(res, &str, TokenType::BYTE));
  EXPECT_EQ(str, s4_);

  // UTF-8 compilation of the same.
  ASSERT_TRUE(StringCompile(s3_, &res, TokenType::UTF8));
  ASSERT_TRUE(CheckFst(res));
  ASSERT_TRUE(Equal(*s3_utf8_, res));
  ASSERT_TRUE(StringPrint(res, &str, TokenType::UTF8));
  EXPECT_EQ(str, s3_);

  ASSERT_TRUE(StringCompile(s4_, &res, TokenType::UTF8));
  ASSERT_TRUE(CheckFst(res));
  ASSERT_TRUE(Equal(*s4_utf8_, res));
  ASSERT_TRUE(StringPrint(res, &str, TokenType::UTF8));
  EXPECT_EQ(str, s4_);
}

TEST_F(StringTest, SymbolTest) {
  VectorFst<Arc> res;
  std::string str;

  // SymbolTable compilation.
  ASSERT_TRUE(StringCompile(s3_, &res, TokenType::SYMBOL, symbols_.get()));
  ASSERT_TRUE(CheckFst(res));
  ASSERT_TRUE(Equal(*s3_syms_, res));
  ASSERT_TRUE(StringPrint(res, &str, TokenType::SYMBOL, symbols_.get()));
  EXPECT_EQ(str, s3_);
}

TEST_F(StringTest, WeightTest) {
  VectorFst<Arc> s1_res;
  std::string str;

  // Bytestring compilation of ASCII string.
  ASSERT_TRUE(StringCompile(s1_, &s1_res, /*token_type=*/TokenType::BYTE,
                            /*symbols=*/nullptr, /*weight=*/Weight(1)));
  ASSERT_TRUE(CheckFst(s1_res));
  ASSERT_TRUE(StringPrint(s1_res, &str));
  EXPECT_EQ(str, s1_);
}

TEST_F(StringTest, BasicBracketTest) {
  VectorFst<Arc> s5_byte_res;

  // Bytestring compilation.
  ASSERT_TRUE(StringCompile(s5_, &s5_byte_res, TokenType::BYTE));
  ASSERT_TRUE(CheckFst(s5_byte_res));

  VectorFst<Arc> s6_byte_res;
  ASSERT_TRUE(StringCompile(s6_, &s6_byte_res, TokenType::BYTE));
  ASSERT_TRUE(CheckFst(s6_byte_res));

  ASSERT_TRUE(Equal(s5_byte_res, s6_byte_res));

  // UTF-8 compilation (should be exactly the same).
  VectorFst<Arc> s5_utf8_res;
  ASSERT_TRUE(StringCompile(s5_, &s5_utf8_res, TokenType::UTF8));
  ASSERT_TRUE(CheckFst(s5_utf8_res));
  ASSERT_TRUE(Equal(s5_byte_res, s5_utf8_res));

  VectorFst<Arc> s6_utf8_res;
  ASSERT_TRUE(StringCompile(s6_, &s6_utf8_res, TokenType::UTF8));
  ASSERT_TRUE(CheckFst(s6_utf8_res));
  ASSERT_TRUE(Equal(s6_byte_res, s6_utf8_res));

  ASSERT_TRUE(Equal(s5_utf8_res, s6_utf8_res));
}

TEST_F(StringTest, MixedBracketTest) {
  VectorFst<Arc> s7_res;

  // Bytestring compilation.
  ASSERT_TRUE(StringCompile(s7_, &s7_res, TokenType::BYTE));
  ASSERT_TRUE(CheckFst(s7_res));
  ASSERT_TRUE(Equal(*s7_byte_, s7_res));

  // UTF-8 compilation.
  ASSERT_TRUE(StringCompile(s7_, &s7_res, TokenType::UTF8));
  ASSERT_TRUE(CheckFst(s7_res));
  ASSERT_TRUE(Equal(*s7_utf8_, s7_res));
}

TEST_F(StringTest, EscapedBracketTest) {
  VectorFst<Arc> s8_res;

  // Bytestring compilation.
  ASSERT_TRUE(StringCompile(s8_, &s8_res, TokenType::BYTE));
  ASSERT_TRUE(CheckFst(s8_res));
  ASSERT_TRUE(Equal(*s8_byte_, s8_res));

  // UTF-8 compilation.
  ASSERT_TRUE(StringCompile(s8_, &s8_res, TokenType::UTF8));
  ASSERT_TRUE(CheckFst(s8_res));
  ASSERT_TRUE(Equal(*s8_utf8_, s8_res));
}

TEST_F(StringTest, EscapeSequencesFinalBackslashTest) {
  VectorFst<Arc> s10_res;
  std::string str;

  ASSERT_TRUE(StringCompile(s10_, &s10_res, TokenType::BYTE));
  ASSERT_TRUE(CheckFst(s10_res));
  ASSERT_TRUE(StringPrint(s10_res, &str, TokenType::BYTE));
  ASSERT_FALSE(str.empty());
  EXPECT_EQ(str.back(), '\\');
  EXPECT_EQ(str, s10_);

  ASSERT_TRUE(StringCompile(s10_, &s10_res, TokenType::UTF8));
  ASSERT_TRUE(CheckFst(s10_res));
  ASSERT_TRUE(StringPrint(s10_res, &str, TokenType::UTF8));
  ASSERT_FALSE(str.empty());
  EXPECT_EQ(str.back(), '\\');
  EXPECT_EQ(str, s10_);
}

TEST_F(StringTest, EscapeSequencesWhitespaceTest) {
  VectorFst<Arc> s11_res;
  std::string str;
  const std::string expected = "\n\\n\r\\r\t\\t";
  ASSERT_EQ(expected.size(), 9);

  ASSERT_TRUE(StringCompile(s11_, &s11_res, TokenType::BYTE));
  ASSERT_TRUE(CheckFst(s11_res));
  ASSERT_TRUE(StringPrint(s11_res, &str, TokenType::BYTE));
  EXPECT_EQ(str, expected);

  ASSERT_TRUE(StringCompile(s11_, &s11_res, TokenType::UTF8));
  ASSERT_TRUE(CheckFst(s11_res));
  ASSERT_TRUE(StringPrint(s11_res, &str, TokenType::UTF8));
  EXPECT_EQ(str, expected);
}

TEST_F(StringTest, EscapeSequencesMultiplyReachableTest) {
  VectorFst<Arc> s12_res;
  std::string str;
  const std::string expected = "\\a\\a\\#\\#";
  ASSERT_EQ(expected.size(), 8);

  ASSERT_TRUE(StringCompile(s12_, &s12_res, TokenType::BYTE));
  ASSERT_TRUE(CheckFst(s12_res));
  ASSERT_TRUE(StringPrint(s12_res, &str, TokenType::BYTE));
  EXPECT_EQ(str, expected);

  ASSERT_TRUE(StringCompile(s12_, &s12_res, TokenType::UTF8));
  ASSERT_TRUE(CheckFst(s12_res));
  ASSERT_TRUE(StringPrint(s12_res, &str, TokenType::UTF8));
  EXPECT_EQ(str, expected);
}

// Tests the special handling for BOS and EOS tokens.
TEST_F(StringTest, BosEosTest) {
  VectorFst<Arc> s9_res;

  ASSERT_TRUE(StringCompile(s9_, &s9_res));
  ASSERT_TRUE(CheckFst(s9_res));

  // Tests that there is exactly one arc leaving the start state and that it
  // is labeled kBosIndex.
  {
    ArcIterator<VectorFst<Arc>> aiter(s9_res, s9_res.Start());
    ASSERT_FALSE(aiter.Done());
    const auto& arc = aiter.Value();
    EXPECT_EQ(kBosIndex, arc.ilabel);
    EXPECT_EQ(kBosIndex, arc.olabel);
    aiter.Next();
    EXPECT_TRUE(aiter.Done());
  }

  // Tests that there is exactly one arc leaving the penultimate state and that
  // it is labeled kEosIndex.
  {
    ArcIterator<VectorFst<Arc>> aiter(s9_res, s9_res.NumStates() - 2);
    ASSERT_FALSE(aiter.Done());
    const auto& arc = aiter.Value();
    EXPECT_EQ(kEosIndex, arc.ilabel);
    EXPECT_EQ(kEosIndex, arc.olabel);
    aiter.Next();
    EXPECT_TRUE(aiter.Done());
  }
}

TEST_F(StringTest, GeneratedSymbolsTest) {
  const auto& symbols = GeneratedSymbols();
  EXPECT_TRUE(symbols.Member("año"));
}

// This is the only unit which uses or manualates the defaults.
TEST_F(StringTest, GetDefaultTest) {
  VectorFst<Arc> res;
  std::string str;

  // Bytestring compilation.
  ASSERT_TRUE(StringCompile(s3_, &res, GetDefaultTokenType()));
  ASSERT_TRUE(CheckFst(res));
  ASSERT_TRUE(Equal(*s3_byte_, res));
  ASSERT_TRUE(StringPrint(res, &str, GetDefaultTokenType()));
  EXPECT_EQ(str, s3_);
  EXPECT_EQ(GetDefaultSymbols(), nullptr);

  // UTF-8 compilation.
  PushDefaults(TokenType::UTF8);
  ASSERT_TRUE(StringCompile(s3_, &res, GetDefaultTokenType()));
  ASSERT_TRUE(CheckFst(res));
  ASSERT_TRUE(Equal(*s3_utf8_, res));
  ASSERT_TRUE(StringPrint(res, &str, GetDefaultTokenType()));
  EXPECT_EQ(str, s3_);
  EXPECT_EQ(GetDefaultSymbols(), nullptr);

  // Symbol table compilation.
  {
    SymbolTable symbols;
    symbols.AddSymbol("<eps>");
    symbols.AddSymbol("a");
    symbols.AddSymbol("b");
    symbols.AddSymbol("c");
    PushDefaults(TokenType::SYMBOL, &symbols);
    const std::string symstr = "a a b b";
    ASSERT_TRUE(StringCompile(symstr, &res, GetDefaultTokenType(),
                              GetDefaultSymbols()));
    ASSERT_TRUE(CheckFst(res));
    ASSERT_TRUE(
        StringPrint(res, &str, GetDefaultTokenType(), GetDefaultSymbols()));
    EXPECT_EQ(str, symstr);
    EXPECT_TRUE(CompatSymbols(&symbols, GetDefaultSymbols()));
  }

  // UTF-8 compilation via pop.
  PopDefaults();
  ASSERT_TRUE(StringCompile(s3_, &res, GetDefaultTokenType()));
  ASSERT_TRUE(CheckFst(res));
  ASSERT_TRUE(Equal(*s3_utf8_, res));
  ASSERT_TRUE(StringPrint(res, &str, GetDefaultTokenType()));
  EXPECT_EQ(str, s3_);
  EXPECT_EQ(GetDefaultSymbols(), nullptr);

  // Bytestring compilation via pop.
  PopDefaults();
  ASSERT_TRUE(StringCompile(s3_, &res, GetDefaultTokenType()));
  ASSERT_TRUE(CheckFst(res));
  ASSERT_TRUE(Equal(*s3_byte_, res));
  ASSERT_TRUE(StringPrint(res, &str, GetDefaultTokenType()));
  EXPECT_EQ(str, s3_);
  EXPECT_EQ(GetDefaultSymbols(), nullptr);
}

// Tests round-trip weighted string printing.
TEST_F(StringTest, WeightedPrintingTest) {
  std::string str;
  Weight typed_weight;
  float weight;

  ASSERT_TRUE(StringPrint(*weighted_, &str, &typed_weight));
  ASSERT_TRUE(StringPrint(*weighted_, &str, &weight));
  EXPECT_EQ(str, "утка");
  EXPECT_NEAR(weight, 14, 1e-8);
}

// Tests with the scripting API.
TEST_F(StringTest, StringCompileScript) {
  namespace s = fst::script;
  const std::string arc_type = "standard";
  const auto& one = s::WeightClass::One("tropical");
  std::string str;

  // Bytestring compilation.
  s::VectorFstClass s1_res(arc_type);
  ASSERT_TRUE(StringCompile(s1_, &s1_res, TokenType::BYTE, nullptr, one));
  ASSERT_TRUE(CheckFst(s1_res));
  ASSERT_TRUE(StringPrint(s1_res, &str));
  EXPECT_EQ(str, s1_);

  s::VectorFstClass s2_res(arc_type);
  ASSERT_TRUE(StringCompile(s2_, &s2_res, TokenType::BYTE, nullptr, one));
  ASSERT_TRUE(CheckFst(s2_res));
  ASSERT_TRUE(StringPrint(s2_res, &str));
  EXPECT_EQ(str, s1_);  // Compares against "canonical" form of string.

  ASSERT_TRUE(Equal(s1_res, s2_res));

  // SymbolTable compilation.
  s::VectorFstClass s3_res(arc_type);
  ASSERT_TRUE(
      StringCompile(s3_, &s3_res, TokenType::SYMBOL, symbols_.get(), one));
  ASSERT_TRUE(CheckFst(s3_res));
  ASSERT_TRUE(StringPrint(s3_res, &str, TokenType::SYMBOL, symbols_.get()));
  EXPECT_EQ(str, s3_);
}

}  // namespace
}  // namespace fst
