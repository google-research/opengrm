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

#include "opengrm/thrax/walker/util/function/stringfst.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "absl/base/casts.h"
#include "absl/log/check.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/ast/fst-node.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

class StringFstTest : public ::testing::Test {
 protected:
  using Arc = ::fst::StdArc;
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  void TearDown() override { StringFst<Arc>::ClearSymbolLabelMapForTest(); }

  virtual void RunByteTest(const std::string& input,
                           const std::vector<int64_t>& golden) {
    auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>();
    args->push_back(
        std::make_unique<DataType>(static_cast<int>(StringFstNode::BYTE)));
    args->push_back(std::make_unique<DataType>(input));
    RunTest(std::move(args), golden);
  }

  virtual void RunTest(
      std::unique_ptr<std::vector<std::unique_ptr<DataType>>> args,
      const std::vector<int64_t>& golden) {
    auto result = func_.Run(std::move(args));
    auto* fst =
        absl::down_cast<MutableTransducer*>(*result->get<Transducer*>());
    std::vector<int64_t> labels;
    GatherLabels(*fst, &labels);
    EXPECT_EQ(golden, labels);
  }

  virtual void GatherLabels(const Transducer& fst,
                            std::vector<int64_t>* labels) {
    labels->clear();
    auto p = fst.Start();
    while (fst.Final(p) == Arc::Weight::Zero()) {
      ::fst::ArcIterator<Transducer> aiter(fst, p);
      CHECK(!aiter.Done());
      const auto& arc = aiter.Value();
      labels->push_back(arc.olabel);
      p = arc.nextstate;
    }
  }

  StringFst<Arc> func_;
};

TEST_F(StringFstTest, BasicTest) {
  std::vector<int64_t> golden;
  golden.push_back('a');
  golden.push_back('b');
  golden.push_back('c');
  RunByteTest("abc", golden);
}

TEST_F(StringFstTest, EscapedStuffTest) {
  std::vector<int64_t> golden;
  golden.push_back('a');
  golden.push_back('\\');
  golden.push_back('b');
  golden.push_back('c');
  golden.push_back('\n');
  golden.push_back('\\');
  golden.push_back('\"');
  golden.push_back('\\');
  golden.push_back('[');
  RunByteTest("a\\bc\\n\\\"\\\\\\[", golden);
}

TEST_F(StringFstTest, EasyGeneratedSymbolTest) {
  std::vector<int64_t> golden;
  golden.push_back('a');
  golden.push_back(0xF0000);
  golden.push_back('c');
  RunByteTest("a[temp_symbol]c", golden);
}

TEST_F(StringFstTest, HarderGeneratedSymbolTest) {
  std::vector<int64_t> golden;
  golden.push_back(0xF0000);
  golden.push_back('a');
  golden.push_back('[');
  golden.push_back('b');
  golden.push_back(']');
  golden.push_back('c');
  golden.push_back(0xF0001);
  RunByteTest("[temp_symbol]a\\[b\\]c[symbol_\\]]", golden);
}

TEST_F(StringFstTest, RepeatedGeneratedSymbolTest) {
  std::vector<int64_t> golden;
  golden.push_back(0xF0000);  // aa
  golden.push_back(0xF0001);  // bb
  golden.push_back(0xF0000);  // aa
  golden.push_back(0xF0001);  // bb
  golden.push_back(0xF0002);  // cc
  RunByteTest("[aa][bb][aa][bb][cc]", golden);
}

TEST_F(StringFstTest, LabelLookupTest) {
  std::vector<int64_t> golden;
  golden.push_back(0xF0000);
  golden.push_back(0xF0001);
  RunByteTest("[aa][bb]", golden);

  int64_t label = 0;
  EXPECT_TRUE(StringFst<Arc>::SymbolToGeneratedLabel("aa", &label));
  EXPECT_EQ(0xF0000, label);
  EXPECT_TRUE(StringFst<Arc>::SymbolToGeneratedLabel("bb", &label));
  EXPECT_EQ(0xF0001, label);
  EXPECT_FALSE(StringFst<Arc>::SymbolToGeneratedLabel("cc", &label));
}

TEST_F(StringFstTest, BoundaryLabelTest) {
  std::vector<int64_t> golden;
  golden.push_back(0xF8FE);
  golden.push_back(0xF8FF);
  RunByteTest("[BOS][EOS]", golden);
  // Note that SymbolToGeneratedLabel() tests above will return false on EOS,
  // BOS since they are not stored.
}

}  // namespace function
}  // namespace thrax
