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

#include "opengrm/string/prefix_tree.h"

#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/icu.h"
#include "openfst/lib/project.h"
#include "openfst/lib/properties.h"
#include "openfst/lib/rmepsilon.h"
#include "openfst/lib/string.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

using Arc = StdArc;

class PrefixTreeTest : public ::testing::Test {
 protected:
  PrefixTreeTest()
      : string_compiler_(fst::TokenType::UTF8),
        string_printer_(fst::TokenType::UTF8) {}

  static void AddStrings(TransducerPrefixTree<Arc>* pt, StdVectorFst* fst,
                         absl::string_view in, absl::string_view out) {
    using Label = typename Arc::Label;
    std::vector<Label> unicode_in;
    UTF8StringToLabels(in, &unicode_in);
    std::vector<Label> unicode_out;
    UTF8StringToLabels(out, &unicode_out);
    pt->Add(unicode_in, unicode_out);
    pt->ToFst(fst);
  }

  static void AddStrings(AcceptorPrefixTree<Arc>* pt, StdVectorFst* fst,
                         absl::string_view in) {
    using Label = typename Arc::Label;
    std::vector<Label> unicode_in;
    UTF8StringToLabels(in, &unicode_in);
    // TODO: Note that this still has an interface that accepts 2
    // sequences, but I should change that.
    pt->Add(unicode_in, unicode_in);
    pt->ToFst(fst);
  }

  void ExpectLookupSuccess(const StdVectorFst& fst, absl::string_view in,
                           absl::string_view expected) {
    StdVectorFst in_fst;
    string_compiler_(in, &in_fst);
    StdVectorFst composed;
    Compose(in_fst, fst, &composed);
    Project(&composed, ProjectType::OUTPUT);
    RmEpsilon(&composed);
    std::string out;
    string_printer_(composed, &out);
    EXPECT_EQ(expected, out);
  }

  void ExpectLookupSuccess(const StdVectorFst& fst, absl::string_view in) {
    StdVectorFst in_fst;
    string_compiler_(in, &in_fst);
    StdVectorFst composed;
    Compose(in_fst, fst, &composed);
    EXPECT_NE(kNoStateId, composed.Start());
  }

  void ExpectLookupFailure(const StdVectorFst& fst, absl::string_view in) {
    StdVectorFst in_fst;
    string_compiler_(in, &in_fst);
    StdVectorFst composed;
    Compose(in_fst, fst, &composed);
    EXPECT_EQ(kNoStateId, composed.Start());
  }

  const StringCompiler<Arc> string_compiler_;
  const StringPrinter<Arc> string_printer_;
};

TEST_F(PrefixTreeTest, TestNoOp) {
  // This test prevents a seemingly unused no-op method from being culled by
  // DeadCode.
  Arc::StateId s = 0;
  EXPECT_EQ(nullptr,
            internal::PrefixTreeAcceptorPolicy<Arc>::ONode::LookupOrInsertChild(
                0, &s));
}

TEST_F(PrefixTreeTest, TransducerPrefixTree) {
  TransducerPrefixTree<Arc> pt;
  StdVectorFst fst;

  EXPECT_TRUE(fst.Properties(kAcceptor, true));
  AddStrings(&pt, &fst, "", "empty");
  EXPECT_TRUE(fst.Properties(kNotAcceptor, true));
  EXPECT_TRUE(fst.Properties(kString, true));
  AddStrings(&pt, &fst, "foo", "fu");
  EXPECT_TRUE(fst.Properties(kNotString, true));
  AddStrings(&pt, &fst, "foobar", "fubar");
  AddStrings(&pt, &fst, "quux", "kwuks");
  AddStrings(&pt, &fst, "foo", "fu");  // Add this a second time.
  AddStrings(&pt, &fst, "福", "fú");
  AddStrings(&pt, &fst, "read", "r iy d");
  EXPECT_TRUE(fst.Properties(kIDeterministic, true));
  AddStrings(&pt, &fst, "read", "r eh d");
  EXPECT_TRUE(fst.Properties(kNonIDeterministic, true));

  ExpectLookupSuccess(fst, "", "empty");
  ExpectLookupSuccess(fst, "foo", "fu");
  ExpectLookupSuccess(fst, "foobar", "fubar");
  ExpectLookupSuccess(fst, "quux", "kwuks");
  ExpectLookupSuccess(fst, "福", "fú");

  ExpectLookupFailure(fst, "fo");
  ExpectLookupFailure(fst, "foob");

  // These hold in general, by construction:
  EXPECT_EQ(pt.NumStates(), fst.NumStates());
  EXPECT_TRUE(fst.Properties(kILabelSorted, true));
  EXPECT_TRUE(fst.Properties(kOLabelSorted, true));
  EXPECT_TRUE(fst.Properties(kAcyclic, true));
  EXPECT_TRUE(fst.Properties(kAccessible, true));
  EXPECT_TRUE(fst.Properties(kCoAccessible, true));

  pt.Clear();
  pt.ToFst(&fst);
  EXPECT_TRUE(fst.Properties(kAcceptor, true));
  ExpectLookupFailure(fst, "");
}

TEST_F(PrefixTreeTest, AcceptorPrefixTree) {
  AcceptorPrefixTree<Arc> pt;
  StdVectorFst fst;

  AddStrings(&pt, &fst, "");
  EXPECT_TRUE(fst.Properties(kString, true));
  AddStrings(&pt, &fst, "foo");
  EXPECT_TRUE(fst.Properties(kNotString, true));
  AddStrings(&pt, &fst, "foobar");
  AddStrings(&pt, &fst, "quux");
  AddStrings(&pt, &fst, "foo");  // Add this a second time.
  AddStrings(&pt, &fst, "福");
  AddStrings(&pt, &fst, "read");
  AddStrings(&pt, &fst, "read");

  ExpectLookupSuccess(fst, "");
  ExpectLookupSuccess(fst, "foo");
  ExpectLookupSuccess(fst, "foobar");
  ExpectLookupSuccess(fst, "quux");
  ExpectLookupSuccess(fst, "福");

  ExpectLookupFailure(fst, "fo");
  ExpectLookupFailure(fst, "foob");

  // These hold in general, by construction:
  EXPECT_EQ(pt.NumStates(), fst.NumStates());
  EXPECT_TRUE(fst.Properties(kAcceptor, true));
  EXPECT_TRUE(fst.Properties(kNoEpsilons, true));
  EXPECT_TRUE(fst.Properties(kIDeterministic, true));
  EXPECT_TRUE(fst.Properties(kODeterministic, true));
  EXPECT_TRUE(fst.Properties(kILabelSorted, true));
  EXPECT_TRUE(fst.Properties(kOLabelSorted, true));
  EXPECT_TRUE(fst.Properties(kAcyclic, true));
  EXPECT_TRUE(fst.Properties(kAccessible, true));
  EXPECT_TRUE(fst.Properties(kCoAccessible, true));

  pt.Clear();
  pt.ToFst(&fst);
  EXPECT_TRUE(fst.Properties(kAcceptor, true));
  ExpectLookupFailure(fst, "");
}

}  // namespace
}  // namespace fst
