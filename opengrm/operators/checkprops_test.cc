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

#include "opengrm/operators/checkprops.h"

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

using Arc = StdArc;

TEST(CheckPropsTest, ValidUnweightedAcceptor) {
  VectorFst<Arc> fst;
  int s0 = fst.AddState();
  fst.SetStart(s0);
  int s1 = fst.AddState();
  fst.SetFinal(s1, Arc::Weight::One());
  fst.AddArc(s0, Arc(1, 1, Arc::Weight::One(), s1));

  EXPECT_TRUE(internal::CheckUnweightedAcceptor(fst, "TestOp", "fst"));
}

TEST(CheckPropsTest, InvalidWeightedAcceptor) {
  VectorFst<Arc> fst;
  int s0 = fst.AddState();
  fst.SetStart(s0);
  int s1 = fst.AddState();
  fst.SetFinal(s1, Arc::Weight::One());
  fst.AddArc(s0, Arc(1, 1, Arc::Weight(2.0), s1));

  EXPECT_FALSE(internal::CheckUnweightedAcceptor(fst, "TestOp", "fst"));
}

TEST(CheckPropsTest, InvalidTransducer) {
  VectorFst<Arc> fst;
  int s0 = fst.AddState();
  fst.SetStart(s0);
  int s1 = fst.AddState();
  fst.SetFinal(s1, Arc::Weight::One());
  // Not an acceptor because input label (1) != output label (2).
  fst.AddArc(s0, Arc(1, 2, Arc::Weight::One(), s1));

  EXPECT_FALSE(internal::CheckUnweightedAcceptor(fst, "TestOp", "fst"));
}

}  // namespace
}  // namespace fst
