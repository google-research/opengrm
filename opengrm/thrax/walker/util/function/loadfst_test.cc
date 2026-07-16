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

#include "opengrm/thrax/walker/util/function/loadfst.h"

#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

template <typename Arc>
class LoadFstTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  void SetUp() override { absl::SetFlag(&FLAGS_indir, ::testing::TempDir()); }

  LoadFst<Arc> func_;
};

using ArcTypes = ::testing::Types<::fst::StdArc, ::fst::LogArc>;
TYPED_TEST_SUITE(LoadFstTest, ArcTypes, );

// Tests that LoadFst correctly loads an FST from a file.
TYPED_TEST(LoadFstTest, TestLoad) {
  typename TestFixture::MutableTransducer golden;
  auto p = golden.AddState();
  auto q = golden.AddState();
  golden.SetStart(p);
  golden.EmplaceArc(p, 'a', 'a', q);
  golden.SetFinal(q);

  const std::string filename = "temp_fst.bin";
  const std::string full_path =
      ::fst::JoinPath(::testing::TempDir(), filename);
  EXPECT_TRUE(golden.Write(full_path));

  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(1);
  (*args)[0] = std::make_unique<DataType>(filename);
  auto result_data = this->func_.Run(std::move(args));
  ASSERT_NE(result_data, nullptr);
  auto* result =
      *result_data->template get<typename TestFixture::Transducer*>();

  EXPECT_TRUE(::fst::Equal(*result, golden));

  std::remove(full_path.c_str());
}

}  // namespace function
}  // namespace thrax
