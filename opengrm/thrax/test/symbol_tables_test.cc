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

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "openfst/extensions/far/far.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"

namespace thrax {
namespace {

using ::fst::CompatSymbols;
using ::fst::FarReader;
using ::fst::kNoSymbol;
using ::fst::StdArc;
using ::fst::StdVectorFst;

class SymbolTablesTest : public ::testing::Test {
 protected:
  SymbolTablesTest() {
    far_path_ = fst::JoinPath(
        std::string("."),
        "opengrm/thrax/test/testdata/symbol_tables/"
        "test2.far");
  }

  std::string far_path_;
};

TEST_F(SymbolTablesTest, BasicTest) {
  std::unique_ptr<FarReader<StdArc>> far(FarReader<StdArc>::Open(far_path_));
  for (; !far->Done(); far->Next()) {
    if (far->GetKey() == "*StringFstSymbolTable") continue;
    // The only other exported fst is test2.
    ASSERT_EQ(far->GetKey(), "test2");
    StdVectorFst fst(*far->GetFst());
    const auto* isyms = fst.InputSymbols();
    const auto* osyms = fst.OutputSymbols();
    ASSERT_FALSE(isyms == nullptr);
    ASSERT_FALSE(osyms == nullptr);
    ASSERT_TRUE(CompatSymbols(isyms, osyms));
    ASSERT_NE(isyms->Find("label1"), kNoSymbol);
    ASSERT_NE(isyms->Find("label2"), kNoSymbol);
    break;
  }
}

}  // namespace
}  // namespace thrax
