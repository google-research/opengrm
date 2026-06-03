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

#include "opengrm/paths/paths.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/string.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "openfst/script/fst-class.h"
#include "opengrm/paths/pathsscript.h"

namespace fst {
namespace {

using Label = typename StdArc::Label;

using ::testing::ContainerEq;
using ::testing::IsEmpty;

class PathsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string word_fsa_name = fst::JoinPath(
        std::string("."),
        "opengrm/paths/testdata/word_fsa.fst");
    const std::string utf8_fsa_name = fst::JoinPath(
        std::string("."),
        "opengrm/paths/testdata/utf8_fsa.fst");
    const std::string byte_fsa_name = fst::JoinPath(
        std::string("."),
        "opengrm/paths/testdata/byte_fsa.fst");
    const std::string byte_fsa2_name = fst::JoinPath(
        std::string("."),
        "opengrm/paths/testdata/byte_fsa2.fst");
    const std::string byte_fst_name = fst::JoinPath(
        std::string("."),
        "opengrm/paths/testdata/byte_fst.fst");
    const std::string cheese_fst_name = fst::JoinPath(
        std::string("."),
        "opengrm/paths/testdata/cheeseshop.fst");
    const std::string word_symbols_name =
        fst::JoinPath(std::string("."),
                       "opengrm/paths/testdata/word.syms");
    word_fsa_.reset(StdVectorFst::Read(word_fsa_name));
    utf8_fsa_.reset(StdVectorFst::Read(utf8_fsa_name));
    byte_fsa_.reset(StdVectorFst::Read(byte_fsa_name));
    byte_fsa2_.reset(StdVectorFst::Read(byte_fsa2_name));
    byte_fst_.reset(StdVectorFst::Read(byte_fst_name));
    cheese_fst_.reset(StdVectorFst::Read(cheese_fst_name));
    word_syms_.reset(SymbolTable::ReadText(word_symbols_name));
  }

  void BytesToLabels(absl::string_view str, std::vector<Label>* labels) {
    labels->resize(str.size());
    std::copy(str.begin(), str.end(), labels->begin());
  }

  std::unique_ptr<StdVectorFst> word_fsa_;
  std::unique_ptr<StdVectorFst> utf8_fsa_;
  std::unique_ptr<StdVectorFst> byte_fsa_;
  std::unique_ptr<StdVectorFst> byte_fsa2_;
  std::unique_ptr<StdVectorFst> byte_fst_;
  std::unique_ptr<StdVectorFst> cheese_fst_;
  std::unique_ptr<SymbolTable> word_syms_;
};

TEST_F(PathsTest, PathIteratorTest) {
  // Contains four strings, including the empty string, some internal final
  // states, and internal weights, which have to get appropriately summed.
  StdPathIterator paths(*byte_fsa_);
  ASSERT_FALSE(paths.Done());
  EXPECT_THAT(paths.ILabels(), IsEmpty());
  EXPECT_EQ(paths.Weight().Value(), 1);
  paths.Next();
  ASSERT_FALSE(paths.Done());
  {
    std::string estring = "your father smelled of εlderberries";
    std::vector<Label> evector;
    ASSERT_TRUE(internal::ConvertStringToLabels(estring, TokenType::BYTE,
                                                nullptr, kNoLabel, &evector));
    EXPECT_THAT(paths.ILabels(), ContainerEq(evector));
    EXPECT_EQ(paths.Weight().Value(), 7);
  }
  paths.Next();
  ASSERT_FALSE(paths.Done());
  {
    std::string estring = "your mother";
    std::vector<Label> evector(estring.begin(), estring.end());
    EXPECT_THAT(paths.ILabels(), ContainerEq(evector));
    EXPECT_EQ(paths.Weight().Value(), 0);
  }
  paths.Next();
  ASSERT_FALSE(paths.Done());
  {
    std::string estring = "your mother was a hamster";
    std::vector<Label> evector(estring.begin(), estring.end());
    EXPECT_THAT(paths.ILabels(), ContainerEq(evector));
    EXPECT_EQ(paths.Weight().Value(), 5);
  }
  paths.Next();
  EXPECT_TRUE(paths.Done());
}

TEST_F(PathsTest, EpsilonPath) {
  StdVectorFst epsilon;
  epsilon.AddState();
  epsilon.SetStart(0);
  epsilon.SetFinal(0, StdArc::Weight::One());
  StdPathIterator iter(epsilon);
  ASSERT_FALSE(iter.Done());
  EXPECT_EQ(0, iter.ILabels().size());
  iter.Next();
  EXPECT_TRUE(iter.Done());
}

TEST_F(PathsTest, NullFstWithStart) {
  StdVectorFst null;
  null.AddState();
  null.SetStart(0);
  StdPathIterator iter(null);
  ASSERT_TRUE(iter.Done());
}

TEST_F(PathsTest, NullFstWithoutStart) {
  StdVectorFst null;
  StdPathIterator iter(null);
  ASSERT_TRUE(iter.Done());
}

TEST_F(PathsTest, ByteFsaTest) {
  // Basic byte acceptor. Contains four strings, including the empty string,
  // some internal final states, and internal weights, which have to get
  // appropriately summed.
  StdStringPathIterator paths(*byte_fsa_);
  ASSERT_FALSE(paths.Done());
  std::string result;
  paths.IString(&result);
  EXPECT_EQ(result, "");
  EXPECT_EQ(paths.Weight().Value(), 1);
  paths.Next();
  ASSERT_FALSE(paths.Done());
  paths.IString(&result);
  EXPECT_EQ(result, "your father smelled of εlderberries");
  EXPECT_EQ(paths.Weight().Value(), 7);
  paths.Next();
  ASSERT_FALSE(paths.Done());
  paths.IString(&result);
  EXPECT_EQ(result, "your mother");
  EXPECT_EQ(paths.Weight().Value(), 0);
  paths.Next();
  ASSERT_FALSE(paths.Done());
  paths.IString(&result);
  EXPECT_EQ(result, "your mother was a hamster");
  EXPECT_EQ(paths.Weight().Value(), 5);
  paths.Next();
  EXPECT_TRUE(paths.Done());
}

TEST_F(PathsTest, ByteFsa2Test) {
  // Like the above, but without the empty string.
  StdStringPathIterator paths(*byte_fsa2_);
  ASSERT_FALSE(paths.Done());
  std::string result;
  paths.IString(&result);
  EXPECT_EQ(result, "your father smelled of εlderberries");
  EXPECT_EQ(paths.Weight().Value(), 7);
  paths.Next();
  ASSERT_FALSE(paths.Done());
  paths.IString(&result);
  EXPECT_EQ(result, "your mother");
  EXPECT_EQ(paths.Weight().Value(), 0);
  paths.Next();
  ASSERT_FALSE(paths.Done());
  paths.IString(&result);
  EXPECT_EQ(result, "your mother was a hamster");
  EXPECT_EQ(paths.Weight().Value(), 5);
  paths.Next();
  EXPECT_TRUE(paths.Done());
}

TEST_F(PathsTest, UTF8FsaTest) {
  // Same as above, but this with the arcs encoding Unicode code points rather
  // than bytes.
  StdStringPathIterator paths(*utf8_fsa_, TokenType::UTF8, nullptr);
  ASSERT_FALSE(paths.Done());
  std::string result;
  paths.IString(&result);
  EXPECT_EQ(result, "");
  EXPECT_EQ(paths.Weight().Value(), 1);
  paths.Next();
  ASSERT_FALSE(paths.Done());
  paths.IString(&result);
  EXPECT_EQ(result, "your father smelled of εlderberries");
  EXPECT_EQ(paths.Weight().Value(), 4);
  paths.Next();
  ASSERT_FALSE(paths.Done());
  paths.IString(&result);
  EXPECT_EQ(result, "your mother");
  EXPECT_EQ(paths.Weight().Value(), 0);
  paths.Next();
  ASSERT_FALSE(paths.Done());
  paths.IString(&result);
  EXPECT_EQ(result, "your mother was a hamster");
  EXPECT_EQ(paths.Weight().Value(), 5);
  paths.Next();
  EXPECT_TRUE(paths.Done());
}

TEST_F(PathsTest, SymbolTest) {
  // Same as above, but the arcs have word labels.
  StdStringPathIterator paths(*word_fsa_, TokenType::SYMBOL, word_syms_.get());
  ASSERT_FALSE(paths.Done());
  std::string result;
  paths.IString(&result);
  EXPECT_EQ(result, "");
  EXPECT_EQ(paths.Weight().Value(), 1);
  paths.Next();
  ASSERT_FALSE(paths.Done());
  paths.IString(&result);
  EXPECT_EQ(result, "your mother");
  EXPECT_EQ(paths.Weight().Value(), 0);
  paths.Next();
  ASSERT_FALSE(paths.Done());
  paths.IString(&result);
  EXPECT_EQ(result, "your mother was a hamster");
  EXPECT_EQ(paths.Weight().Value(), 5);
  paths.Next();
  ASSERT_FALSE(paths.Done());
  paths.IString(&result);
  EXPECT_EQ(result, "your father smelled of elderberries");
  EXPECT_EQ(paths.Weight().Value(), 4);
  paths.Next();
  EXPECT_TRUE(paths.Done());
}

TEST_F(PathsTest, ByteFstTest) {
  // Here we have a transducer, so we should get the input and output paths.
  StdStringPathIterator paths(*byte_fst_);
  ASSERT_FALSE(paths.Done());
  std::string result;
  paths.IString(&result);
  EXPECT_EQ(result, "");
  paths.OString(&result);
  EXPECT_EQ(result, "the cat's eaten it");
  EXPECT_EQ(paths.Weight().Value(), 3.5);
  paths.Next();
  ASSERT_FALSE(paths.Done());
  paths.IString(&result);
  EXPECT_EQ(result, "limburger");
  paths.OString(&result);
  EXPECT_EQ(result, "not as such");
  EXPECT_EQ(paths.Weight().Value(), 1);
  paths.Next();
  ASSERT_FALSE(paths.Done());
  paths.IString(&result);
  EXPECT_EQ(result, "your mother was a hamster");
  paths.OString(&result);
  EXPECT_EQ(result, "your father smelled of εlderberries");
  EXPECT_EQ(paths.Weight().Value(), 5);
  paths.Next();
  EXPECT_TRUE(paths.Done());
}

TEST_F(PathsTest, CheeseFstTest) {
  // This is a large FST containing 119 distinct strings.
  StdStringPathIterator paths(*cheese_fst_);
  std::vector<std::string> strings;
  while (!paths.Done()) {
    strings.emplace_back();
    paths.IString(&strings.back());
    paths.Next();
  }
  EXPECT_EQ(strings.size(), 119);
  EXPECT_EQ(strings[0], "'Ee I were all 'ungry-like!");
}

TEST_F(PathsTest, CheeseScript) {
  // Finally, a test of this all in scriptland.
  namespace s = fst::script;
  s::VectorFstClass cheese_script_fst(*cheese_fst_);
  std::vector<std::string> strings;
  s::StringPathIteratorClass string_paths(cheese_script_fst);
  while (!string_paths.Done()) {
    strings.emplace_back();
    string_paths.IString(&strings.back());
    string_paths.Next();
    EXPECT_EQ(string_paths.Weight().ToString(), "0");
  }
  EXPECT_EQ(strings.size(), 119);
  EXPECT_EQ(strings[0], "'Ee I were all 'ungry-like!");
  // Tests Reset().
  string_paths.Reset();
  strings.clear();
  while (!string_paths.Done()) {
    strings.emplace_back();
    string_paths.IString(&strings.back());
    string_paths.Next();
    EXPECT_EQ(string_paths.Weight().ToString(), "0");
  }
  EXPECT_EQ(strings.size(), 119);
  EXPECT_EQ(strings[0], "'Ee I were all 'ungry-like!");
}

}  // namespace
}  // namespace fst
