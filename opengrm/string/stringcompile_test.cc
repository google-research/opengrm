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

#include "opengrm/string/stringcompile.h"

#include <cstdint>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace fst {
namespace {

using ::testing::Each;
using ::testing::ElementsAre;
using ::testing::Ge;

constexpr static int64_t kMinGeneratedLabel = 0xF0000;

TEST(StringToLabelsTest, Decimal) {
  std::vector<int32_t> labels;
  ASSERT_TRUE(StringToLabels("[32]", &labels));
  EXPECT_THAT(labels, ElementsAre(32));
}

TEST(StringToLabelsTest, Hex) {
  std::vector<int32_t> labels;
  ASSERT_TRUE(StringToLabels("[0x48]", &labels));
  EXPECT_THAT(labels, ElementsAre(0x48));
}

TEST(StringToLabelsTest, CapitalHex) {
  std::vector<int32_t> labels;
  ASSERT_TRUE(StringToLabels("[0X48]", &labels));
  EXPECT_THAT(labels, ElementsAre(0x48));
}

TEST(StringToLabelsTest, Octal) {
  std::vector<int32_t> labels;
  ASSERT_TRUE(StringToLabels("[077]", &labels));
  EXPECT_THAT(labels, ElementsAre(077));
}

TEST(StringToLabelsTest, MultipleSpaceDelimitedNumbers) {
  std::vector<int32_t> labels;
  ASSERT_TRUE(StringToLabels("[0x48 077 29]", &labels));
  EXPECT_THAT(labels, Each(Ge(kMinGeneratedLabel)));
}

TEST(StringToLabelsTest, Zero) {
  std::vector<int32_t> labels;
  ASSERT_TRUE(StringToLabels("[0]", &labels));
  EXPECT_THAT(labels, ElementsAre(0));
}

TEST(StringToLabelsTest, One) {
  std::vector<int32_t> labels;
  ASSERT_TRUE(StringToLabels("[1]", &labels));
  EXPECT_THAT(labels, ElementsAre(1));
}

TEST(StringToLabelsTest, OctalZero) {
  std::vector<int32_t> labels;
  ASSERT_TRUE(StringToLabels("[00]", &labels));
  EXPECT_THAT(labels, ElementsAre(0));
}

TEST(StringToLabelsTest, HexZero) {
  std::vector<int32_t> labels;
  ASSERT_TRUE(StringToLabels("[0x0]", &labels));
  EXPECT_THAT(labels, ElementsAre(0));
}

TEST(StringToLabelsTest, MalformedHexZero) {
  std::vector<int32_t> labels;
  ASSERT_TRUE(StringToLabels("[0x]", &labels));
  EXPECT_THAT(labels, ElementsAre(Ge(kMinGeneratedLabel)));
}

TEST(StringToLabelsTest, NegativeDecimal) {
  std::vector<int32_t> labels;
  ASSERT_TRUE(StringToLabels("[-3]", &labels));
  EXPECT_THAT(labels, ElementsAre(-3));
}

TEST(StringToLabelsTest, NegativeHex) {
  std::vector<int32_t> labels;
  ASSERT_TRUE(StringToLabels("[-0x1A]", &labels));
  EXPECT_THAT(labels, ElementsAre(-0x1A));
}

TEST(StringToLabelsTest, NegativeOctal) {
  std::vector<int32_t> labels;
  ASSERT_TRUE(StringToLabels("[-017]", &labels));
  EXPECT_THAT(labels, ElementsAre(-017));
}

TEST(StringToLabelsTest, GeneratedLabelConsistency) {
  int32_t flames_label;
  {
    std::vector<int32_t> labels;
    ASSERT_TRUE(StringToLabels("[flames]", &labels));
    EXPECT_THAT(labels, ElementsAre(Ge(kMinGeneratedLabel)));
    flames_label = labels[0];
  }
  {
    std::vector<int32_t> labels;
    ASSERT_TRUE(StringToLabels("ab[flames]cd", &labels));
    EXPECT_THAT(labels, ElementsAre('a', 'b', flames_label, 'c', 'd'));
  }
}

}  // namespace
}  // namespace fst
