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

#include "opengrm/thrax/walker/util/function/symboltable.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "absl/strings/string_view.h"
#include "opengrm/thrax/walker/util/function/temp_file.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/symbol-table.h"
#include "opengrm/thrax/walker/util/datatype.h"

namespace thrax {
namespace function {

const char kSymbolTableFilename[] = "symtab";
const char kSymbolTableBuffer[] = "fairy\t1\npolarbear\t2\n";

template <typename Arc>
class SymbolTableTest : public ::testing::Test {
 protected:
  SymbolTableTest() : file_holder_(kSymbolTableFilename, kSymbolTableBuffer) {}

  TempFile file_holder_;
  SymbolTable<Arc> func_;
};

using ArcTypes = ::testing::Types<::fst::StdArc, ::fst::LogArc>;
TYPED_TEST_SUITE(SymbolTableTest, ArcTypes);

TYPED_TEST(SymbolTableTest, TestSymbolTable) {
  auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>(1);
  (*args)[0] = std::make_unique<DataType>(this->file_holder_.path());
  auto result_data = this->func_.Run(std::move(args));
  const auto* syms = result_data->template get<::fst::SymbolTable>();
  EXPECT_EQ(2, syms->NumSymbols());
  EXPECT_EQ(1, syms->Find("fairy"));
  EXPECT_EQ(2, syms->Find("polarbear"));
  EXPECT_EQ(-1, syms->Find("stapler"));
  EXPECT_EQ("fairy", syms->Find(1));
  EXPECT_EQ("polarbear", syms->Find(2));
  EXPECT_EQ("", syms->Find(3));
}

}  // namespace function
}  // namespace thrax
