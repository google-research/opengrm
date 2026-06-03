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

#include "opengrm/thrax/walker/util/function/stringfile.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "absl/strings/string_view.h"
#include "opengrm/thrax/walker/util/function/temp_file.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

const char kInputFilename[] = "input.txt";
const char kInputContent[] = "aa\nab\nba";

const char kInputFilename2[] = "input2.txt";
const char kInputContent2[] = "kimchi\t김치";

const char kInputFilename3[] = "input3.txt";
const char kInputContent3[] = "kimchi\t김치\t0.5";

template <typename Arc>
class StringFileTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  StringFileTest()
      : input_file_holder_(kInputFilename, kInputContent),
        input_file_holder2_(kInputFilename2, kInputContent2),
        input_file_holder3_(kInputFilename3, kInputContent3) {
    auto start = golden_fst_.AddState();
    golden_fst_.SetStart(start);

    // "aa" and "ab"
    auto a = golden_fst_.AddState();
    golden_fst_.EmplaceArc(start, 'a', 'a', a);
    auto aa = golden_fst_.AddState();
    golden_fst_.EmplaceArc(a, 'a', 'a', aa);
    golden_fst_.SetFinal(aa);
    auto ab = golden_fst_.AddState();
    golden_fst_.EmplaceArc(a, 'b', 'b', ab);
    golden_fst_.SetFinal(ab);

    // "ba"
    auto b = golden_fst_.AddState();
    golden_fst_.EmplaceArc(start, 'b', 'b', b);
    auto ba = golden_fst_.AddState();
    golden_fst_.EmplaceArc(b, 'a', 'a', ba);
    golden_fst_.SetFinal(ba);

    // "kimchi" -> "김치". 44608 and 52824 are code points for the two hangul
    // syllables.
    start = golden_fst2_.AddState();
    golden_fst2_.SetStart(start);
    auto k = golden_fst2_.AddState();
    golden_fst2_.EmplaceArc(start, 'k', 44608, k);
    auto ki = golden_fst2_.AddState();
    golden_fst2_.EmplaceArc(k, 'i', 52824, ki);
    auto kim = golden_fst2_.AddState();
    golden_fst2_.EmplaceArc(ki, 'm', 0, kim);
    auto kimc = golden_fst2_.AddState();
    golden_fst2_.EmplaceArc(kim, 'c', 0, kimc);
    auto kimch = golden_fst2_.AddState();
    golden_fst2_.EmplaceArc(kimc, 'h', 0, kimch);
    auto kimchi = golden_fst2_.AddState();
    golden_fst2_.EmplaceArc(kimch, 'i', 0, kimchi);
    golden_fst2_.SetFinal(kimchi);

    // Weighted version of the above.
    start = golden_fst3_.AddState();
    golden_fst3_.SetStart(start);
    k = golden_fst3_.AddState();
    golden_fst3_.EmplaceArc(start, 'k', 44608, k);
    ki = golden_fst3_.AddState();
    golden_fst3_.EmplaceArc(k, 'i', 52824, ki);
    kim = golden_fst3_.AddState();
    golden_fst3_.EmplaceArc(ki, 'm', 0, kim);
    kimc = golden_fst3_.AddState();
    golden_fst3_.EmplaceArc(kim, 'c', 0, kimc);
    kimch = golden_fst3_.AddState();
    golden_fst3_.EmplaceArc(kimc, 'h', 0, kimch);
    kimchi = golden_fst3_.AddState();
    golden_fst3_.EmplaceArc(kimch, 'i', 0, kimchi);
    golden_fst3_.SetFinal(kimchi, typename Arc::Weight(0.5));
  }

  TempFile input_file_holder_;
  TempFile input_file_holder2_;
  TempFile input_file_holder3_;
  MutableTransducer golden_fst_;
  MutableTransducer golden_fst2_;
  MutableTransducer golden_fst3_;

  StringFile<Arc> func_;
};

using ArcTypes = ::testing::Types<::fst::StdArc, ::fst::LogArc>;
TYPED_TEST_SUITE(StringFileTest, ArcTypes);

TYPED_TEST(StringFileTest, TestBuild) {
  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(1);
  (*args)[0] = std::make_unique<DataType>(this->input_file_holder_.path());
  auto result = this->func_.Run(std::move(args));
  auto* fst = *result->template get<typename TestFixture::Transducer*>();
  EXPECT_TRUE(::fst::Equal(*fst, this->golden_fst_));

  auto args2 = std::make_unique<std::vector<std::unique_ptr<DataType>>>(3);
  (*args2)[0] = std::make_unique<DataType>(this->input_file_holder2_.path());
  (*args2)[1] = std::make_unique<DataType>("byte");
  (*args2)[2] = std::make_unique<DataType>("utf8");
  auto result2 = this->func_.Run(std::move(args2));
  fst = *result2->template get<typename TestFixture::Transducer*>();
  EXPECT_TRUE(::fst::Equal(*fst, this->golden_fst2_));

  auto args3 = std::make_unique<std::vector<std::unique_ptr<DataType>>>(3);
  (*args3)[0] = std::make_unique<DataType>(this->input_file_holder3_.path());
  (*args3)[1] = std::make_unique<DataType>("byte");
  (*args3)[2] = std::make_unique<DataType>("utf8");
  auto result3 = this->func_.Run(std::move(args3));
  fst = *result3->template get<typename TestFixture::Transducer*>();
  EXPECT_TRUE(::fst::Equal(*fst, this->golden_fst3_));
}

}  // namespace function
}  // namespace thrax
