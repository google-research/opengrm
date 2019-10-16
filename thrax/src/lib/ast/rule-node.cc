#include <thrax/rule-node.h>

#include <thrax/identifier-node.h>
#include <thrax/walker.h>

namespace thrax {

RuleNode::RuleNode(IdentifierNode* name, Node* rhs, ExportStatus exp)
    : Node(), name_(name), rhs_(rhs), export_(exp) {}

RuleNode::~RuleNode() {
  delete name_;
  delete rhs_;
}

IdentifierNode* RuleNode::GetName() const { return name_; }

Node* RuleNode::Get() const { return rhs_; }

bool RuleNode::ShouldExport() const { return export_ == EXPORT; }

void RuleNode::Accept(AstWalker* walker) { walker->Visit(this); }

}  // namespace thrax
