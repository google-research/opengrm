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

#include "opengrm/thrax/walker/util/resource-map.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "gtest/gtest.h"
#include "absl/base/casts.h"
#include "absl/functional/bind_front.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "openfst/lib/vector-fst.h"

using ::fst::ArcIterator;
using ::fst::Fst;
using ::fst::StdArc;
using ::fst::VectorFst;

namespace thrax {
namespace {

class Squarer {
 public:
  explicit Squarer(int i) : i_(i) {}

  int GetSquare() const { return i_ * i_; }

 private:
  int i_;
};

class ResourceMapTest : public ::testing::Test {
 protected:
  void SetUp() override { map_ = std::make_unique<ResourceMap>(); }

  std::unique_ptr<ResourceMap> map_;
};

TEST_F(ResourceMapTest, RegressionTest) {
  // Insert two Squarer objects.
  EXPECT_TRUE(map_->Insert("one", std::make_unique<Squarer>(1)));
  EXPECT_TRUE(map_->Insert("two", std::make_unique<Squarer>(200)));
  EXPECT_FALSE(map_->Insert("two", std::make_unique<Squarer>(2)));
  ASSERT_EQ(2, map_->Size());

  // Check that the squaring works.
  EXPECT_EQ(1, map_->Get<Squarer>("one")->GetSquare());
  EXPECT_EQ(4, map_->Get<Squarer>("two")->GetSquare());

  // Replace the "two" squarer with a string.
  EXPECT_FALSE(map_->Insert("two", std::make_unique<std::string>("two")));
  ASSERT_EQ(2, map_->Size());

  // Check things still evaluate correctly.
  EXPECT_EQ(1, map_->Get<Squarer>("one")->GetSquare());
  EXPECT_EQ("two", *map_->Get<std::string>("two"));

  // Check that our contains functions are correct.
  EXPECT_TRUE(map_->Contains("one"));
  EXPECT_TRUE(map_->Contains("two"));
  EXPECT_FALSE(map_->Contains("polar bears are awesome"));

  // Check that our typed contains functions are also correct.
  EXPECT_FALSE(map_->ContainsType<Squarer*>("one"));
  EXPECT_TRUE(map_->ContainsType<Squarer>("one"));
  EXPECT_FALSE(map_->ContainsType<std::string*>("one"));
  EXPECT_TRUE(map_->ContainsType<std::string>("two"));
  EXPECT_FALSE(map_->ContainsType<double>("one"));
  EXPECT_FALSE(map_->ContainsType<std::string>("fairies are awesome"));

  // Check that we can erase elements.
  EXPECT_TRUE(map_->Erase("one"));
  EXPECT_FALSE(map_->Erase("one"));  // Should now be gone already.
  ASSERT_EQ(1, map_->Size());
  EXPECT_EQ("two", *map_->Get<std::string>("two"));

  // Check for what happens if we try to get invalid elements.
  EXPECT_EQ(nullptr, map_->Get<Squarer>("one"));
  EXPECT_DEATH(map_->Get<int>("two"), "original_type == requested_type");

  // If we clear the map, it should be empty.
  map_->Clear();
  ASSERT_EQ(0, map_->Size());

  // Let's add something back and let TearDown delete the map.
  map_->Insert("quarter", std::make_unique<double>(.25));
  EXPECT_EQ(.25, *map_->Get<double>("quarter"));

  std::string s = "abcdefg";
  map_->InsertWithDeleter("sss", &s, nullptr);
}

template <typename T>
void DeleteArray(T* p) {
  delete[] p;
}

TEST_F(ResourceMapTest, DeletionTest) {
  int* ints = new int[5];
  std::function<void()> deleter = absl::bind_front(&DeleteArray<int>, ints);
  map_->InsertWithDeleter("ints", ints, deleter);

  int* i = new int;
  map_->InsertWithDeleter("int", i, nullptr);
  delete i;
}

TEST_F(ResourceMapTest, FstTest) {
  // Create a vector FST.
  auto vfst = std::make_unique<VectorFst<StdArc>>();
  StdArc::StateId q = vfst->AddState();
  vfst->SetStart(q);
  StdArc::StateId r = vfst->AddState();
  vfst->AddArc(q, StdArc('a', 'b', StdArc::Weight::One(), r));
  vfst->SetFinal(r, StdArc::Weight::One());

  // Add it to the map.
  map_->Insert("fst", absl::implicit_cast<std::unique_ptr<Fst<StdArc>>>(
                          std::move(vfst)));

  // Retrieve the FST as a default Fst instance.
  Fst<StdArc>* new_fst = map_->Get<Fst<StdArc>>("fst");
  EXPECT_EQ(0, new_fst->Start());
  EXPECT_EQ(StdArc::Weight::Zero(), new_fst->Final(0));
  EXPECT_EQ(StdArc::Weight::One(), new_fst->Final(1));
  EXPECT_EQ(1, new_fst->NumArcs(0));
  EXPECT_EQ(0, new_fst->NumArcs(1));

  // Perform the down_cast to VectorFst and call some of its functions.
  VectorFst<StdArc>* new_vfst =
      absl::down_cast<fst::VectorFst<StdArc>*>(new_fst);
  new_vfst->AddArc(0, StdArc('c', 'd', StdArc::Weight::One(), 1));

  // Check that we have two arcs (albeit with invalid probabilities) on the
  // original (non-vector) Fst pointer.
  ArcIterator<Fst<StdArc>> aiter(*new_fst, 0);
  EXPECT_EQ('a', aiter.Value().ilabel);
  EXPECT_EQ('b', aiter.Value().olabel);
  EXPECT_EQ(StdArc::Weight::One(), aiter.Value().weight);
  EXPECT_EQ(1, aiter.Value().nextstate);
  aiter.Next();
  ASSERT_FALSE(aiter.Done());
  EXPECT_EQ('c', aiter.Value().ilabel);
  EXPECT_EQ('d', aiter.Value().olabel);
  EXPECT_EQ(StdArc::Weight::One(), aiter.Value().weight);
  EXPECT_EQ(1, aiter.Value().nextstate);
  aiter.Next();
  ASSERT_TRUE(aiter.Done());
}

TEST_F(ResourceMapTest, ConstTest) {
  // Create a const object and put it into the map.
  map_->Insert("fairy", std::make_unique<const std::string>("awesome"));

  // If we put in a const string, we can only pull out a const string.
  const std::string* const_str = map_->Get<const std::string>("fairy");
  EXPECT_EQ("awesome", *const_str);

  // Pulling a non-const string will not work.
  EXPECT_DEATH(map_->Get<std::string>("fairy"),
               "original_type == requested_type");

  // Similarly, if we insert a non-const string, we can't get a const string
  // out.  Feel free to store the non-const pointer into a const variable.
  map_->Insert("mutable fairy", std::make_unique<std::string>("fantastic"));
  const_str =
      map_->Get<std::string>("mutable fairy");  // Store into const pointer.
  EXPECT_EQ("fantastic", *const_str);
  EXPECT_DEATH(map_->Get<const std::string>("mutable fairy"),
               "original_type == requested_type");
}

TEST_F(ResourceMapTest, ReleaseTest) {
  map_->Insert("fairy", std::make_unique<std::string>("awesome"));
  EXPECT_TRUE(map_->Contains("fairy"));

  auto released = map_->Release<std::string>("fairy");
  EXPECT_FALSE(map_->Contains("fairy"));
}

}  // namespace
}  // namespace thrax
