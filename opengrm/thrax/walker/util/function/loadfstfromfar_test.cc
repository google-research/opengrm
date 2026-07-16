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

#include "opengrm/thrax/walker/util/function/loadfstfromfar.h"

#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "openfst/extensions/far/far-type.h"
#include "openfst/extensions/far/far-writer.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/equal.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

template <typename Arc>
class LoadFstFromFarTest : public ::testing::Test {
 protected:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;

  void SetUp() override { absl::SetFlag(&FLAGS_indir, ::testing::TempDir()); }

  LoadFstFromFar<Arc> func_;
};

using ArcTypes = ::testing::Types<::fst::StdArc, ::fst::LogArc>;
TYPED_TEST_SUITE(LoadFstFromFarTest, ArcTypes, );

// Tests that LoadFstFromFar correctly loads an FST from a FAR archive.
TYPED_TEST(LoadFstFromFarTest, TestLoadFromFar) {
  using Arc = TypeParam;
  typename TestFixture::MutableTransducer golden;
  auto p = golden.AddState();
  auto q = golden.AddState();
  golden.SetStart(p);
  golden.EmplaceArc(p, 'a', 'a', q);
  golden.SetFinal(q);

  const std::string far_filename = "temp_archive.far";
  const std::string full_far_path =
      ::fst::JoinPath(::testing::TempDir(), far_filename);
  const std::string fst_name = "golden_fst";

  std::unique_ptr<::fst::FarWriter<Arc>> writer(
      ::fst::FarWriter<Arc>::Create(full_far_path, ::fst::FarType::STTABLE));
  ASSERT_NE(writer, nullptr);
  writer->Add(fst_name, golden);
  writer.reset();

  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(2);
  (*args)[0] = std::make_unique<DataType>(far_filename);
  (*args)[1] = std::make_unique<DataType>(fst_name);
  auto result_data = this->func_.Run(std::move(args));
  ASSERT_NE(result_data, nullptr);
  auto* result =
      *result_data->template get<typename TestFixture::Transducer*>();

  EXPECT_TRUE(::fst::Equal(*result, golden));

  std::remove(full_far_path.c_str());
}

}  // namespace function
}  // namespace thrax
