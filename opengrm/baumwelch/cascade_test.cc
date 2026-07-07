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

// Unit tests for Baum-Welch cascade objects.

#include "opengrm/baumwelch/cascade.h"

#include "gtest/gtest.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/cache.h"
#include "openfst/lib/compose.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"

namespace fst {
namespace {

// Helper to construct a simple 2-state string FST:
// s0 --(ilabel:olabel)--> s1 (final)
VectorFst<StdArc> MakeSimpleFst(int ilabel, int olabel, double weight = 0.0) {
  VectorFst<StdArc> fst;
  const auto s0 = fst.AddState();
  fst.SetStart(s0);
  const auto s1 = fst.AddState();
  fst.AddArc(s0, StdArc(ilabel, olabel, StdArc::Weight(weight), s1));
  fst.SetFinal(s1, StdArc::Weight::One());
  return fst;
}

TEST(SimpleCascadeTest, ComposesInputModelOutput) {
  // Input: "1" (1:1)
  const VectorFst<StdArc> input = MakeSimpleFst(1, 1);
  // Model: transducer mapping 1 -> 10
  const VectorFst<StdArc> model = MakeSimpleFst(1, 10);
  // Output: "10" (10:10)
  const VectorFst<StdArc> output = MakeSimpleFst(10, 10);

  const SimpleCascade<StdArc> cascade(input, output, model);
  const ComposeFst<StdArc>& composed = cascade.GetFst();

  EXPECT_NE(composed.Start(), kNoStateId);
  const VectorFst<StdArc> vfst(composed);
  EXPECT_EQ(vfst.NumStates(), 2);
}

TEST(SimpleCascadeTest, CustomOptions) {
  const VectorFst<StdArc> input = MakeSimpleFst(1, 1);
  const VectorFst<StdArc> model = MakeSimpleFst(1, 10);
  const VectorFst<StdArc> output = MakeSimpleFst(10, 10);

  const CacheOptions co_cache_opts(/*gc=*/true, /*gc_limit=*/1024);
  const CacheOptions ico_cache_opts(/*gc=*/false, /*gc_limit=*/0);
  const CascadeOptions opts(co_cache_opts, ico_cache_opts);

  const SimpleCascade<StdArc> cascade(input, output, model, opts);
  const ComposeFst<StdArc>& composed = cascade.GetFst();

  EXPECT_NE(composed.Start(), kNoStateId);
  const VectorFst<StdArc> vfst(composed);
  EXPECT_EQ(vfst.NumStates(), 2);
}

TEST(ChannelStateCascadeTest, MapsComposedStateToModelChannelState) {
  // Model FST: s0 --(1:10)--> s1 --(2:20)--> s2 (final)
  VectorFst<StdArc> model;
  const auto m0 = model.AddState();
  model.SetStart(m0);
  const auto m1 = model.AddState();
  const auto m2 = model.AddState();
  model.AddArc(m0, StdArc(1, 10, StdArc::Weight::One(), m1));
  model.AddArc(m1, StdArc(2, 20, StdArc::Weight::One(), m2));
  model.SetFinal(m2, StdArc::Weight::One());

  // Input FST: s0 --(1:1)--> s1 --(2:2)--> s2 (final)
  VectorFst<StdArc> input;
  const auto i0 = input.AddState();
  input.SetStart(i0);
  const auto i1 = input.AddState();
  const auto i2 = input.AddState();
  input.AddArc(i0, StdArc(1, 1, StdArc::Weight::One(), i1));
  input.AddArc(i1, StdArc(2, 2, StdArc::Weight::One(), i2));
  input.SetFinal(i2, StdArc::Weight::One());

  // Output FST: s0 --(10:10)--> s1 --(20:20)--> s2 (final)
  VectorFst<StdArc> output;
  const auto o0 = output.AddState();
  output.SetStart(o0);
  const auto o1 = output.AddState();
  const auto o2 = output.AddState();
  output.AddArc(o0, StdArc(10, 10, StdArc::Weight::One(), o1));
  output.AddArc(o1, StdArc(20, 20, StdArc::Weight::One(), o2));
  output.SetFinal(o2, StdArc::Weight::One());

  const ChannelStateCascade<StdArc> cascade(input, output, model);
  const ComposeFst<StdArc>& composed = cascade.GetFst();

  const auto start_state = composed.Start();
  EXPECT_NE(start_state, kNoStateId);

  // Start state of composed FST should map to start state of model (m0 = 0).
  EXPECT_EQ(cascade.ChannelState(start_state), m0);
}

TEST(ChannelStateCascadeTest, CustomOptions) {
  const VectorFst<StdArc> input = MakeSimpleFst(1, 1);
  const VectorFst<StdArc> model = MakeSimpleFst(1, 10);
  const VectorFst<StdArc> output = MakeSimpleFst(10, 10);

  const CacheOptions co_cache_opts(/*gc=*/true, /*gc_limit=*/2048);
  const CacheOptions ico_cache_opts(/*gc=*/false, /*gc_limit=*/0);
  const CascadeOptions opts(co_cache_opts, ico_cache_opts);

  const ChannelStateCascade<StdArc> cascade(input, output, model, opts);
  const ComposeFst<StdArc>& composed = cascade.GetFst();

  EXPECT_NE(composed.Start(), kNoStateId);
  EXPECT_EQ(cascade.ChannelState(composed.Start()), 0);
}

}  // namespace
}  // namespace fst
