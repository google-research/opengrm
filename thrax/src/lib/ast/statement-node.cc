#include <thrax/statement-node.h>

#include <thrax/walker.h>

namespace thrax {

StatementNode::StatementNode(StatementNodeType type)
    : Node(), type_(type), statement_(nullptr) {}

StatementNode::~StatementNode() { delete statement_; }

StatementNode::StatementNodeType StatementNode::GetType() const {
  return type_;
}

void StatementNode::Set(Node* statement) { statement_ = statement; }

Node* StatementNode::Get() const { return statement_; }

void StatementNode::Accept(AstWalker* walker) { walker->Visit(this); }

}  // namespace thrax
