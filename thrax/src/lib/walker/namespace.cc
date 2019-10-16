#include <thrax/namespace.h>

#include <map>
#include <string>
#include <vector>

#include <thrax/identifier-node.h>
#include <thrax/algo/resource-map.h>
#include <thrax/compat/utils.h>
#include <thrax/compat/stlfunctions.h>

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
  STLDeleteContainerPairSecondPointers(alias_namespace_map_.begin(),
                                            alias_namespace_map_.end());
  if (owns_resources_) delete resources_;
}

Namespace* Namespace::AddSubNamespace(const std::string& filename,
                                      const std::string& alias) {
  auto* new_namespace = new Namespace(filename, resources_);
  if (!InsertIfNotPresent(&alias_namespace_map_, alias, new_namespace)) {
    LOG(FATAL) << "Cannot reuse the same alias for two files: "
               << alias << " in  " << filename;
  }
  return new_namespace;
}

void Namespace::PushLocalEnvironment() {
  local_env_.push_back(new ResourceMap());
}

void Namespace::PopLocalEnvironment() {
  delete local_env_.back();
  local_env_.pop_back();
}

int Namespace::LocalEnvironmentDepth() const { return local_env_.size(); }

bool Namespace::EraseLocal(const std::string& identifier) {
  return local_env_.back()->Erase(identifier);
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
    auto** next = FindOrNull(alias_namespace_map_, namespace_name);
    if (next) {
      ++(*identifier_nspos);
      return (*next)->ResolveNamespaceInternal(identifier, identifier_nspos);
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
  return thrax::StringCat(filename_, "/", identifier_name);
}

}  // namespace thrax
