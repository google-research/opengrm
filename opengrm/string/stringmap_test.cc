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

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/string.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/script/equal.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/verify.h"
#include "openfst/script/weight-class.h"
#include "opengrm/string/stringmapscript.h"

namespace fst {
namespace {

using Arc = StdArc;
using Weight = Arc::Weight;

class StringMapTest : public ::testing::Test {
 protected:
  void SetUp() final {
    const std::string testdir = fst::JoinPath(
        std::string("."), "opengrm/string/testdata");

    lines_ = {{"[Bel Paese]", "Sorry"},
              {"Cheddar"},
              {"Caithness", "Pont-l'Évêque", ".666"},
              {"Pont-l'Évêque", "Camembert"}};

    const script::WeightClass weight_one(Weight::One());
    const script::WeightClass weight_point_666(Weight(.666));
    typed_lines_ = {{"[Bel Paese]", "Sorry", weight_one},
                    {"Cheddar", "Cheddar", weight_one},
                    {"Caithness", "Pont-l'Évêque", weight_point_666},
                    {"Pont-l'Évêque", "Camembert", weight_one}};

    const std::string b2b_name = fst::JoinPath(testdir, "b2b.fst");
    const std::string b2s_name = fst::JoinPath(testdir, "b2s.fst");
    const std::string b2u_name = fst::JoinPath(testdir, "b2u.fst");
    const std::string u2u_name = fst::JoinPath(testdir, "u2u.fst");
    const std::string symbols_name = fst::JoinPath(testdir, "sym.map");

    b2b_.reset(VectorFst<Arc>::Read(b2b_name));
    b2s_.reset(VectorFst<Arc>::Read(b2s_name));
    b2u_.reset(VectorFst<Arc>::Read(b2u_name));
    u2u_.reset(VectorFst<Arc>::Read(u2u_name));
    map_name_ = fst::JoinPath(testdir, "str.map");

    symbols_.reset(SymbolTable::ReadText(symbols_name));
  }

  std::string map_name_;
  std::vector<std::vector<std::string>> lines_;
  std::vector<std::tuple<std::string, std::string, script::WeightClass>>
      typed_lines_;
  std::unique_ptr<VectorFst<Arc>> b2b_;
  std::unique_ptr<VectorFst<Arc>> b2s_;
  std::unique_ptr<VectorFst<Arc>> b2u_;
  std::unique_ptr<VectorFst<Arc>> u2u_;
  std::unique_ptr<SymbolTable> symbols_;
};

TEST_F(StringMapTest, ByteToByteMapTest) {
  namespace s = fst::script;
  const s::VectorFstClass b2b(*b2b_);
  s::VectorFstClass b2b_res("standard");
  ASSERT_TRUE(StringMapCompile(lines_, &b2b_res));
  EXPECT_TRUE(Verify(b2b_res));
  EXPECT_TRUE(Equal(b2b, b2b_res));
}

TEST_F(StringMapTest, ByteToSymbolMapTest) {
  namespace s = fst::script;
  const s::VectorFstClass b2s(*b2s_);
  s::VectorFstClass b2s_res("standard");
  ASSERT_TRUE(StringMapCompile(lines_, &b2s_res, TokenType::BYTE,
                               TokenType::SYMBOL, nullptr, symbols_.get()));
  EXPECT_TRUE(Verify(b2s_res));
  EXPECT_TRUE(Equal(b2s, b2s_res));
}

TEST_F(StringMapTest, ByteToUtf8MapTest) {
  namespace s = fst::script;
  const s::VectorFstClass b2u(*b2u_);
  s::VectorFstClass b2u_res("standard");
  ASSERT_TRUE(
      StringMapCompile(lines_, &b2u_res, TokenType::BYTE, TokenType::UTF8));
  EXPECT_TRUE(Verify(b2u_res));
  EXPECT_TRUE(Equal(b2u, b2u_res));
}

TEST_F(StringMapTest, Utf8ToUtf8MapTest) {
  namespace s = fst::script;
  const s::VectorFstClass u2u(*u2u_);
  s::VectorFstClass u2u_res("standard");
  ASSERT_TRUE(
      StringMapCompile(lines_, &u2u_res, TokenType::UTF8, TokenType::UTF8));
  EXPECT_TRUE(Verify(u2u_res));
  EXPECT_TRUE(Equal(u2u, u2u_res));
}

TEST_F(StringMapTest, ByteToByteTypedMapTest) {
  namespace s = fst::script;
  const s::VectorFstClass b2b(*b2b_);
  s::VectorFstClass b2b_res("standard");
  ASSERT_TRUE(StringMapCompile(typed_lines_, &b2b_res));
  EXPECT_TRUE(Verify(b2b_res));
  EXPECT_TRUE(Equal(b2b, b2b_res));
}

TEST_F(StringMapTest, ByteToSymbolTypedMapTest) {
  namespace s = fst::script;
  const s::VectorFstClass b2s(*b2s_);
  s::VectorFstClass b2s_res("standard");
  ASSERT_TRUE(StringMapCompile(typed_lines_, &b2s_res, TokenType::BYTE,
                               TokenType::SYMBOL, nullptr, symbols_.get()));
  EXPECT_TRUE(Verify(b2s_res));
  EXPECT_TRUE(Equal(b2s, b2s_res));
}

TEST_F(StringMapTest, ByteToUtf8TypedMapTest) {
  namespace s = fst::script;
  const s::VectorFstClass b2u(*b2u_);
  s::VectorFstClass b2u_res("standard");
  ASSERT_TRUE(StringMapCompile(typed_lines_, &b2u_res, TokenType::BYTE,
                               TokenType::UTF8));
  EXPECT_TRUE(Verify(b2u_res));
  EXPECT_TRUE(Equal(b2u, b2u_res));
}

TEST_F(StringMapTest, Utf8ToUtf8TypedMapTest) {
  namespace s = fst::script;
  const s::VectorFstClass u2u(*u2u_);
  s::VectorFstClass u2u_res("standard");
  ASSERT_TRUE(StringMapCompile(typed_lines_, &u2u_res, TokenType::UTF8,
                               TokenType::UTF8));
  EXPECT_TRUE(Verify(u2u_res));
  EXPECT_TRUE(Equal(u2u, u2u_res));
}

TEST_F(StringMapTest, ByteToByteFileTest) {
  namespace s = fst::script;
  const s::VectorFstClass b2b(*b2b_);
  s::VectorFstClass b2b_res("standard");
  ASSERT_TRUE(StringFileCompile(map_name_, &b2b_res));
  EXPECT_TRUE(Verify(b2b_res));
  EXPECT_TRUE(Equal(b2b, b2b_res));
}

TEST_F(StringMapTest, ByteToSymbolFileTest) {
  namespace s = fst::script;
  const s::VectorFstClass b2s(*b2s_);
  s::VectorFstClass b2s_res("standard");
  ASSERT_TRUE(StringFileCompile(map_name_, &b2s_res, TokenType::BYTE,
                                TokenType::SYMBOL, nullptr, symbols_.get()));
  EXPECT_TRUE(Verify(b2s_res));
  EXPECT_TRUE(Equal(b2s, b2s_res));
}

TEST_F(StringMapTest, ByteToUtf8FileTest) {
  namespace s = fst::script;
  const s::VectorFstClass b2u(*b2u_);
  s::VectorFstClass b2u_res("standard");
  ASSERT_TRUE(
      StringFileCompile(map_name_, &b2u_res, TokenType::BYTE, TokenType::UTF8));
  EXPECT_TRUE(Verify(b2u_res));
  EXPECT_TRUE(Equal(b2u, b2u_res));
}

TEST_F(StringMapTest, Utf8ToUtf8FileTest) {
  namespace s = fst::script;
  const s::VectorFstClass u2u(*u2u_);
  s::VectorFstClass u2u_res("standard");
  ASSERT_TRUE(
      StringFileCompile(map_name_, &u2u_res, TokenType::UTF8, TokenType::UTF8));
  EXPECT_TRUE(Verify(u2u_res));
  EXPECT_TRUE(Equal(u2u, u2u_res));
}

class StringMapAcceptorTest : public ::testing::Test {
 protected:
  void SetUp() final {
    const std::string testdir = fst::JoinPath(
        std::string("."), "opengrm/string/testdata");

    lines_ = {{"papacy"}, {"paper", "paper", "3"},
              {"pepper"}, {"pen", "pen", "4"},
              {"résumé"}, {"pencil"},
              {"paper"}};

    const script::WeightClass weight_one(Weight::One());
    const script::WeightClass weight_point_three(Weight(3));
    const script::WeightClass weight_point_four(Weight(4));
    typed_lines_ = {{"papacy", "papacy", weight_one},
                    {"paper", "paper", weight_point_three},
                    {"pepper", "pepper", weight_one},
                    {"pen", "pen", weight_point_four},
                    {"résumé", "résumé", weight_one},
                    {"pencil", "pencil", weight_one},
                    {"paper", "paper", weight_one}};

    const std::string acceptor_b2b_name =
        fst::JoinPath(testdir, "acceptor_b2b.fst");
    const std::string acceptor_u2b_name =
        fst::JoinPath(testdir, "acceptor_u2b.fst");
    const std::string acceptor_u2u_name =
        fst::JoinPath(testdir, "acceptor_u2u.fst");
    const std::string acceptor_u2s_name =
        fst::JoinPath(testdir, "acceptor_u2s.fst");
    const std::string acceptor_s2s_name =
        fst::JoinPath(testdir, "acceptor_s2s.fst");
    fsa_b2b_.reset(VectorFst<Arc>::Read(acceptor_b2b_name));
    fsa_u2b_.reset(VectorFst<Arc>::Read(acceptor_u2b_name));
    fsa_u2u_.reset(VectorFst<Arc>::Read(acceptor_u2u_name));
    fsa_u2s_.reset(VectorFst<Arc>::Read(acceptor_u2s_name));
    fsa_s2s_.reset(VectorFst<Arc>::Read(acceptor_s2s_name));

    std::string symbols_name = fst::JoinPath(testdir, "sym_acceptor.map");
    map_name_ = fst::JoinPath(testdir, "str_acceptor.map");
    symbols_.reset(SymbolTable::ReadText(symbols_name));
  }

  std::unique_ptr<VectorFst<Arc>> fsa_b2b_;
  std::unique_ptr<VectorFst<Arc>> fsa_u2b_;
  std::unique_ptr<VectorFst<Arc>> fsa_u2u_;
  std::unique_ptr<VectorFst<Arc>> fsa_u2s_;
  std::unique_ptr<VectorFst<Arc>> fsa_s2s_;
  std::string map_name_;
  std::vector<std::vector<std::string>> lines_;
  std::vector<std::tuple<std::string, std::string, script::WeightClass>>
      typed_lines_;
  std::unique_ptr<SymbolTable> symbols_;
};

TEST_F(StringMapAcceptorTest, AcceptorFileByteToByteTest) {
  namespace s = fst::script;
  const s::VectorFstClass fsa(*fsa_b2b_);
  s::VectorFstClass fsa_res("standard");
  ASSERT_TRUE(
      StringFileCompile(map_name_, &fsa_res, TokenType::BYTE, TokenType::BYTE));
  EXPECT_TRUE(Verify(fsa_res));
  EXPECT_EQ(fsa_res.Properties(kAcceptor, true), kAcceptor);
  EXPECT_TRUE(Equal(fsa, fsa_res));
}

TEST_F(StringMapAcceptorTest, AcceptorMapByteToByteTest) {
  namespace s = fst::script;
  const s::VectorFstClass fsa(*fsa_b2b_);
  s::VectorFstClass fsa_res("standard");
  ASSERT_TRUE(
      StringMapCompile(lines_, &fsa_res, TokenType::BYTE, TokenType::BYTE));
  EXPECT_TRUE(Verify(fsa_res));
  EXPECT_EQ(fsa_res.Properties(kAcceptor, true), kAcceptor);
  EXPECT_TRUE(Equal(fsa, fsa_res));
}

TEST_F(StringMapAcceptorTest, AcceptorTypedMapByteToByteTest) {
  namespace s = fst::script;
  const s::VectorFstClass fsa(*fsa_b2b_);
  s::VectorFstClass fsa_res("standard");
  ASSERT_TRUE(StringMapCompile(typed_lines_, &fsa_res, TokenType::BYTE,
                               TokenType::BYTE));
  EXPECT_TRUE(Verify(fsa_res));
  EXPECT_EQ(fsa_res.Properties(kAcceptor, true), kAcceptor);
  EXPECT_TRUE(Equal(fsa, fsa_res));
}

TEST_F(StringMapAcceptorTest, AcceptorFileUtf8ToByteTest) {
  namespace s = fst::script;
  const s::VectorFstClass fsa(*fsa_u2b_);
  s::VectorFstClass fsa_res("standard");
  ASSERT_TRUE(
      StringFileCompile(map_name_, &fsa_res, TokenType::UTF8, TokenType::BYTE));
  EXPECT_TRUE(Verify(fsa_res));
  EXPECT_EQ(fsa_res.Properties(kNotAcceptor, true), kNotAcceptor);
  EXPECT_TRUE(Equal(fsa, fsa_res));
}

TEST_F(StringMapAcceptorTest, AcceptorMapUtf8ToByteTest) {
  namespace s = fst::script;
  const s::VectorFstClass fsa(*fsa_u2b_);
  s::VectorFstClass fsa_res("standard");
  ASSERT_TRUE(
      StringMapCompile(lines_, &fsa_res, TokenType::UTF8, TokenType::BYTE));
  EXPECT_TRUE(Verify(fsa_res));
  EXPECT_EQ(fsa_res.Properties(kNotAcceptor, true), kNotAcceptor);
  EXPECT_TRUE(Equal(fsa, fsa_res));
}

TEST_F(StringMapAcceptorTest, AcceptorTypedMapUtf8ToByteTest) {
  namespace s = fst::script;
  const s::VectorFstClass fsa(*fsa_u2b_);
  s::VectorFstClass fsa_res("standard");
  ASSERT_TRUE(StringMapCompile(typed_lines_, &fsa_res, TokenType::UTF8,
                               TokenType::BYTE));
  EXPECT_TRUE(Verify(fsa_res));
  EXPECT_EQ(fsa_res.Properties(kNotAcceptor, true), kNotAcceptor);
  EXPECT_TRUE(Equal(fsa, fsa_res));
}

TEST_F(StringMapAcceptorTest, AcceptorFileUtf8ToUtf8Test) {
  namespace s = fst::script;
  const s::VectorFstClass fsa(*fsa_u2u_);
  s::VectorFstClass fsa_res("standard");
  ASSERT_TRUE(
      StringFileCompile(map_name_, &fsa_res, TokenType::UTF8, TokenType::UTF8));
  EXPECT_TRUE(Verify(fsa_res));
  EXPECT_EQ(fsa_res.Properties(kAcceptor, true), kAcceptor);
  EXPECT_TRUE(Equal(fsa, fsa_res));
}

TEST_F(StringMapAcceptorTest, AcceptorMapUtf8ToUtf8Test) {
  namespace s = fst::script;
  const s::VectorFstClass fsa(*fsa_u2u_);
  s::VectorFstClass fsa_res("standard");
  ASSERT_TRUE(
      StringMapCompile(lines_, &fsa_res, TokenType::UTF8, TokenType::UTF8));
  EXPECT_TRUE(Verify(fsa_res));
  EXPECT_EQ(fsa_res.Properties(kAcceptor, true), kAcceptor);
  EXPECT_TRUE(Equal(fsa, fsa_res));
}

TEST_F(StringMapAcceptorTest, AcceptorTypedMapUtf8ToUtf8Test) {
  namespace s = fst::script;
  const s::VectorFstClass fsa(*fsa_u2u_);
  s::VectorFstClass fsa_res("standard");
  ASSERT_TRUE(StringMapCompile(typed_lines_, &fsa_res, TokenType::UTF8,
                               TokenType::UTF8));
  EXPECT_TRUE(Verify(fsa_res));
  EXPECT_EQ(fsa_res.Properties(kAcceptor, true), kAcceptor);
  EXPECT_TRUE(Equal(fsa, fsa_res));
}

TEST_F(StringMapAcceptorTest, AcceptorFileUtf8ToSymbolsTest) {
  namespace s = fst::script;
  const s::VectorFstClass fsa(*fsa_u2s_);
  s::VectorFstClass fsa_res("standard");
  ASSERT_TRUE(StringFileCompile(map_name_, &fsa_res, TokenType::UTF8,
                                TokenType::SYMBOL, nullptr, symbols_.get()));
  EXPECT_TRUE(Verify(fsa_res));
  EXPECT_EQ(fsa_res.Properties(kNotAcceptor, true), kNotAcceptor);
  EXPECT_TRUE(Equal(fsa, fsa_res));
}

TEST_F(StringMapAcceptorTest, AcceptorMapUtf8ToSymbolsTest) {
  namespace s = fst::script;
  const s::VectorFstClass fsa(*fsa_u2s_);
  s::VectorFstClass fsa_res("standard");
  ASSERT_TRUE(StringMapCompile(lines_, &fsa_res, TokenType::UTF8,
                               TokenType::SYMBOL, nullptr, symbols_.get()));
  EXPECT_TRUE(Verify(fsa_res));
  EXPECT_EQ(fsa_res.Properties(kNotAcceptor, true), kNotAcceptor);
  EXPECT_TRUE(Equal(fsa, fsa_res));
}

TEST_F(StringMapAcceptorTest, AcceptorTypedMapUtf8ToSymbolsTest) {
  namespace s = fst::script;
  const s::VectorFstClass fsa(*fsa_u2s_);
  s::VectorFstClass fsa_res("standard");
  ASSERT_TRUE(StringMapCompile(typed_lines_, &fsa_res, TokenType::UTF8,
                               TokenType::SYMBOL, nullptr, symbols_.get()));
  EXPECT_TRUE(Verify(fsa_res));
  EXPECT_EQ(fsa_res.Properties(kNotAcceptor, true), kNotAcceptor);
  EXPECT_TRUE(Equal(fsa, fsa_res));
}

TEST_F(StringMapAcceptorTest, AcceptorFileSymbolsToSymbolsTest) {
  namespace s = fst::script;
  const s::VectorFstClass fsa(*fsa_s2s_);
  s::VectorFstClass fsa_res("standard");
  ASSERT_TRUE(StringFileCompile(map_name_, &fsa_res, TokenType::SYMBOL,
                                TokenType::SYMBOL, symbols_.get(),
                                symbols_.get()));
  EXPECT_TRUE(Verify(fsa_res));
  EXPECT_EQ(fsa_res.Properties(kAcceptor, true), kAcceptor);
  EXPECT_TRUE(Equal(fsa, fsa_res));
}

TEST_F(StringMapAcceptorTest, AcceptorMapSymbolsToSymbolsTest) {
  namespace s = fst::script;
  const s::VectorFstClass fsa(*fsa_s2s_);
  s::VectorFstClass fsa_res("standard");
  ASSERT_TRUE(StringMapCompile(lines_, &fsa_res, TokenType::SYMBOL,
                               TokenType::SYMBOL, symbols_.get(),
                               symbols_.get()));
  EXPECT_TRUE(Verify(fsa_res));
  EXPECT_EQ(fsa_res.Properties(kAcceptor, true), kAcceptor);
  EXPECT_TRUE(Equal(fsa, fsa_res));
}

TEST_F(StringMapAcceptorTest, AcceptorTypedMapSymbolsToSymbolsTest) {
  namespace s = fst::script;
  const s::VectorFstClass fsa(*fsa_s2s_);
  s::VectorFstClass fsa_res("standard");
  ASSERT_TRUE(StringMapCompile(typed_lines_, &fsa_res, TokenType::SYMBOL,
                               TokenType::SYMBOL, symbols_.get(),
                               symbols_.get()));
  EXPECT_TRUE(Verify(fsa_res));
  EXPECT_EQ(fsa_res.Properties(kAcceptor, true), kAcceptor);
  EXPECT_TRUE(Equal(fsa, fsa_res));
}

}  // namespace
}  // namespace fst
