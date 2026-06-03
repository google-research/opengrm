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

#include "opengrm/thrax/walker/util/namespace.h"

#include <memory>
#include <string>

#include "gtest/gtest.h"
#include "opengrm/thrax/ast/identifier-node.h"

namespace thrax {
namespace {

class NamespaceTest : public ::testing::Test {
 protected:
  void SetUp() override { ns_ = std::make_unique<Namespace>(); }

  std::unique_ptr<Namespace> ns_;
};

TEST_F(NamespaceTest, RegressionTest) {
  // First set up the namespaces. Top includes both fairy and bear, and fairy
  // includes sprite and bear (as well).
  Namespace* ns_fairy = ns_->AddSubNamespace("/fairy", "fairy");
  Namespace* ns_bear = ns_->AddSubNamespace("/bear", "bear");
  Namespace* ns_fairy_sprite = ns_fairy->AddSubNamespace("/sprite", "sprite");
  Namespace* ns_fairy_bear = ns_fairy->AddSubNamespace("/bear", "polar");

  // Check the filenames match up.
  EXPECT_EQ("/fairy", ns_fairy->GetFilename());
  EXPECT_EQ("/bear", ns_bear->GetFilename());
  EXPECT_EQ("/sprite", ns_fairy_sprite->GetFilename());
  EXPECT_EQ("/bear", ns_fairy_bear->GetFilename());

  // ResolveNamespace checks.
  IdentifierNode thing_inode("thing");
  EXPECT_EQ(ns_.get(), ns_->ResolveNamespace(thing_inode));

  IdentifierNode fairy_wing_inode("fairy.wing");
  EXPECT_EQ(ns_fairy, ns_->ResolveNamespace(fairy_wing_inode));

  IdentifierNode bear_tail_inode("bear.tail");
  EXPECT_EQ(ns_bear, ns_->ResolveNamespace(bear_tail_inode));

  IdentifierNode fairy_sprite_pitchfork_inode("fairy.sprite.pitchfork");
  EXPECT_EQ(ns_fairy_sprite,
            ns_->ResolveNamespace(fairy_sprite_pitchfork_inode));

  IdentifierNode fairy_polar_tail_inode("fairy.polar.tail");
  EXPECT_EQ(ns_fairy_bear, ns_->ResolveNamespace(fairy_polar_tail_inode));

  IdentifierNode fairy_panda_bamboo_inode("fairy.panda.bamboo");
  EXPECT_EQ(nullptr, ns_->ResolveNamespace(fairy_panda_bamboo_inode));

  // Insert a few things into our namespace.
  EXPECT_TRUE(ns_->Insert("thing", std::make_unique<int>(0)));
  EXPECT_TRUE(ns_fairy->Insert("wing", std::make_unique<int>(10)));
  EXPECT_TRUE(ns_fairy_sprite->Insert("pitchfork",
                                      std::make_unique<std::string>("stab")));
  EXPECT_TRUE(ns_bear->Insert(
      "tail", std::make_unique<int>(22)));  // Also for fairy.polar.

  // Look up the objects just inserted.
  EXPECT_EQ(0, *ns_->Get<int>(thing_inode));
  EXPECT_EQ(10, *ns_->Get<int>(fairy_wing_inode));
  EXPECT_EQ("stab", *ns_->Get<std::string>(fairy_sprite_pitchfork_inode));
  EXPECT_EQ(22, *ns_->Get<int>(bear_tail_inode));
  EXPECT_EQ(22, *ns_->Get<int>(fairy_polar_tail_inode));  // From bear.

  // These last two should clobber existing data (and remember to free the old
  // values). The second one maps to the same file, so it's also a clobber.
  // Also, check the namespaces are retrieved correctly.
  Namespace* where;
  EXPECT_FALSE(ns_bear->Insert("tail", std::make_unique<int>(21)));
  EXPECT_FALSE(ns_fairy_bear->Insert("tail", std::make_unique<int>(20)));
  EXPECT_EQ(20, *ns_->Get<int>(bear_tail_inode, &where));
  EXPECT_EQ(ns_bear, where);
  EXPECT_EQ(20, *ns_->Get<int>(fairy_polar_tail_inode, &where));
  EXPECT_EQ(ns_fairy_bear, where);

  // Check for some bogus Get() calls.
  EXPECT_EQ(nullptr, ns_->Get<int>(fairy_panda_bamboo_inode));  // Unknown name.
  EXPECT_EQ(nullptr, ns_->Get<double>(fairy_wing_inode));       // Wrong type.

  // Now let's move to local environment tests and check the depth after some
  // pushes and pops.
  EXPECT_EQ(0, ns_->LocalEnvironmentDepth());
  ns_->PushLocalEnvironment();
  EXPECT_EQ(1, ns_->LocalEnvironmentDepth());
  ns_->PushLocalEnvironment();
  EXPECT_EQ(2, ns_->LocalEnvironmentDepth());
  ns_->PopLocalEnvironment();
  EXPECT_EQ(1, ns_->LocalEnvironmentDepth());

  // Add a local variable and immediately read it back again.
  EXPECT_TRUE(ns_->InsertLocal("fairy", std::make_unique<std::string>("dumb")));
  EXPECT_FALSE(ns_->InsertLocal("fairy", std::make_unique<std::string>("fun")));
  IdentifierNode fairy_inode("fairy");
  EXPECT_EQ("fun", *ns_->Get<std::string>(fairy_inode, &where));
  EXPECT_EQ(ns_.get(), where);

  // Add a new variable one level down and see if we can still see fairy.
  ns_->PushLocalEnvironment();
  EXPECT_TRUE(ns_->InsertLocal("bear", std::make_unique<std::string>("cute")));
  IdentifierNode bear_inode("bear");
  EXPECT_EQ("cute", *ns_->Get<std::string>(bear_inode, &where));
  EXPECT_EQ(ns_.get(), where);
  EXPECT_EQ(nullptr, ns_->Get<std::string>(fairy_inode));  // Fairy is hidden.
  ns_->PopLocalEnvironment();
  EXPECT_EQ("fun", *ns_->Get<std::string>(fairy_inode));  // Fairy is back!
  ns_->PopLocalEnvironment();

  // Note that you can't find local variables from a parent namespace.
  ns_fairy->PushLocalEnvironment();
  EXPECT_TRUE(
      ns_fairy->InsertLocal("fairy", std::make_unique<std::string>("magical")));
  EXPECT_EQ("magical", *ns_fairy->Get<std::string>(fairy_inode));
  IdentifierNode fairy_fairy_inode("fairy.fairy");
  EXPECT_EQ(nullptr, ns_->Get<std::string>(fairy_fairy_inode));
  ns_fairy->PopLocalEnvironment();
}

}  // namespace
}  // namespace thrax
