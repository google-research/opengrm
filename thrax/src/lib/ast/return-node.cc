#include <thrax/return-node.h>

#include <thrax/walker.h>

namespace thrax {

ReturnNode::ReturnNode(Node* node) : Node(), node_(node) {}

ReturnNode::~ReturnNode() { delete node_; }

Node* ReturnNode::Get() const { return node_; }

void ReturnNode::Accept(AstWalker* walker) { walker->Visit(this); }

}  // namespace thrax
