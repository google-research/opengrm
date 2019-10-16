#include <thrax/import-node.h>

#include <thrax/identifier-node.h>
#include <thrax/string-node.h>
#include <thrax/walker.h>

namespace thrax {

ImportNode::ImportNode(StringNode* path, IdentifierNode* alias)
    : Node(), path_(path), alias_(alias) {}

ImportNode::~ImportNode() {
  delete path_;
  delete alias_;
}

StringNode* ImportNode::GetPath() const { return path_; }

IdentifierNode* ImportNode::GetAlias() const { return alias_; }

void ImportNode::Accept(AstWalker* walker) { walker->Visit(this); }

}  // namespace thrax
