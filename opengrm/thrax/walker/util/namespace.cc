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

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "absl/base/casts.h"
#include "absl/base/nullability.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "opengrm/thrax/ast/identifier-node.h"
#include "opengrm/thrax/walker/util/resource-map.h"

namespace thrax {

Namespace::Namespace()
    : toplevel_(false), resources_(new ResourceMap()), owns_resources_(true) {}

Namespace::Namespace(absl::string_view filename, ResourceMap* resource_map)
    : toplevel_(false),
      filename_(filename),
      resources_(resource_map),
      owns_resources_(false) {}

Namespace::~Namespace() {
  DCHECK(local_env_.empty());
  if (owns_resources_) delete resources_;
}

Namespace* absl_nullable Namespace::AddSubNamespace(absl::string_view filename,
                                                    absl::string_view alias) {
  // NB: Using `new` rather than `absl::make_unique` due to private constructor.
  auto new_namespace = absl::WrapUnique(new Namespace(filename, resources_));
  auto it_success =
      alias_namespace_map_.emplace(alias, std::move(new_namespace));
  if (!it_success.second) {
    LOG(FATAL) << "Cannot reuse the same alias for two files: " << alias
               << " in  " << filename;
    return nullptr;  // Just to silence -Wreturn-type error.
  } else {
    // NB: This is the value of `new_namespace` now that it's been moved into
    // the alias map.
    return it_success.first->second.get();
  }
}

void Namespace::PushLocalEnvironment() {
  local_env_.push(std::make_unique<ResourceMap>());
}

void Namespace::PopLocalEnvironment() { local_env_.pop(); }

int Namespace::LocalEnvironmentDepth() const { return local_env_.size(); }

bool Namespace::EraseLocal(absl::string_view identifier) {
  return local_env_.top()->Erase(identifier);
}

const Namespace* absl_nullable Namespace::ResolveNamespace(
    const IdentifierNode& identifier) const {
  IdentifierNode::const_iterator where = identifier.begin();
  return ResolveNamespaceInternal(identifier, &where);
}

Namespace* absl_nullable Namespace::ResolveNamespace(
    const IdentifierNode& identifier) {
  IdentifierNode::const_iterator where = identifier.begin();
  return ResolveNamespaceInternal(identifier, &where);
}

const Namespace* absl_nullable Namespace::ResolveNamespaceInternal(
    const IdentifierNode& identifier,
    IdentifierNode::const_iterator* identifier_nspos) const {
  if (*identifier_nspos == identifier.end()) {
    // Here, we're at the end and can just look this up.
    return this;
  } else {
    // Here, we need to look up the next namespace and return that, maybe
    // creating it if requested.
    const std::string& namespace_name = **identifier_nspos;
    auto it = alias_namespace_map_.find(namespace_name);
    if (it != alias_namespace_map_.end()) {
      ++(*identifier_nspos);
      auto& next = it->second;
      return next->ResolveNamespaceInternal(identifier, identifier_nspos);
    } else {
      return nullptr;
    }
  }
}

Namespace* absl_nullable Namespace::ResolveNamespaceInternal(
    const IdentifierNode& identifier,
    IdentifierNode::const_iterator* identifier_nspos) {
  return const_cast<Namespace*>(
      absl::implicit_cast<const Namespace*>(this)->ResolveNamespaceInternal(
          identifier, identifier_nspos));
}

std::string Namespace::GetFilename() const {
  return filename_.empty() ? "<unknown file>" : filename_;
}

void Namespace::SetTopLevel() { toplevel_ = true; }

bool Namespace::IsTopLevel() const { return toplevel_; }

std::string Namespace::ConstructMapName(
    absl::string_view identifier_name) const {
  return absl::StrCat(filename_, "/", identifier_name);
}

}  // namespace thrax
