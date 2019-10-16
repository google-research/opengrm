#include <thrax/function-node.h>

#include <thrax/collection-node.h>
#include <thrax/identifier-node.h>
#include <thrax/walker.h>

namespace thrax {

FunctionNode::FunctionNode(IdentifierNode* name,
                           CollectionNode* arguments,
                           CollectionNode* body)
    : Node(), name_(name), arguments_(arguments), body_(body) {}

FunctionNode::~FunctionNode() {
  delete name_;
  delete arguments_;
  delete body_;
}

IdentifierNode* FunctionNode::GetName() const { return name_; }

CollectionNode* FunctionNode::GetArguments() const { return arguments_; }

CollectionNode* FunctionNode::GetBody() const { return body_; }

void FunctionNode::Accept(AstWalker* walker) { walker->Visit(this); }

}  // namespace thrax
