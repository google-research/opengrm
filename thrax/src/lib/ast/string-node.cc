#include <thrax/string-node.h>

#include <string>

#include <thrax/walker.h>

namespace thrax {

StringNode::StringNode(const std::string& str) : Node(), str_(str) {}

StringNode::~StringNode() {}

const std::string& StringNode::Get() const { return str_; }

void StringNode::Accept(AstWalker* walker) { walker->Visit(this); }

}  // namespace thrax
