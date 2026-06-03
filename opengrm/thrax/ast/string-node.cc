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

#include "opengrm/thrax/ast/string-node.h"

#include <string>

#include "absl/strings/string_view.h"
#include "opengrm/thrax/ast/node.h"
#include "opengrm/thrax/walker/walker.h"

namespace thrax {

StringNode::StringNode(absl::string_view str) : Node(), str_(str) {}

const std::string& StringNode::Get() const { return str_; }

void StringNode::Accept(AstWalker* walker) { walker->Visit(this); }

}  // namespace thrax
