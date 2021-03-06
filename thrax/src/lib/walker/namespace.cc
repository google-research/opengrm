// Copyright 2005-2020 Google LLC
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
//
#include <thrax/namespace.h>

#include <iterator>
#include <map>
#include <string>

#include <thrax/identifier-node.h>
#include <fst/compat.h>

namespace thrax {

Namespace::Namespace()
    : toplevel_(false), resources_(new ResourceMap()), owns_resources_(true) {}

Namespace::Namespace(const std::string& filename, ResourceMap* resource_map)
    : toplevel_(false),
      filename_(filename),
      resources_(resource_map),
      owns_resources_(false) {}

Namespace::~Namespace() {
  CHECK(local_env_.empty());
  if (owns_resources_) delete resources_;
}

Namespace* Namespace::AddSubNamespace(const std::string& filename,
                                      const std::string& alias) {
  // NB: Using `new` rather than `std::make_unique` due to private constructor.
  auto new_namespace = fst::WrapUnique(new Namespace(filename, resources_));
  auto it_success =
      alias_namespace_map_.emplace(alias, std::move(new_namespace));
  if (!it_success.second) {
    LOG(FATAL) << "Cannot reuse the same alias for two files: " << alias
               << " in  " << filename;
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

bool Namespace::EraseLocal(const std::string& identifier) {
  return local_env_.top()->Erase(identifier);
}

Namespace* Namespace::ResolveNamespace(const IdentifierNode& identifier) {
  IdentifierNode::const_iterator where = identifier.begin();
  return ResolveNamespaceInternal(identifier, &where);
}

Namespace* Namespace::ResolveNamespaceInternal(
    const IdentifierNode& identifier,
    IdentifierNode::const_iterator* identifier_nspos) {
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

std::string Namespace::GetFilename() const {
  return filename_.empty() ? "<unknown file>" : filename_;
}

void Namespace::SetTopLevel() { toplevel_ = true; }

bool Namespace::IsTopLevel() const { return toplevel_; }

std::string Namespace::ConstructMapName(
    const std::string& identifier_name) const {
  return ::fst::StrCat(filename_, "/", identifier_name);
}

}  // namespace thrax
