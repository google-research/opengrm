#include <thrax/grammar-node.h>

#include <thrax/collection-node.h>
#include <thrax/walker.h>

namespace thrax {

GrammarNode::GrammarNode(CollectionNode* imports,
                         CollectionNode* functions,
                         CollectionNode* statements)
    : Node(), imports_(imports), functions_(functions),
      statements_(statements) {}

GrammarNode::~GrammarNode() {
  delete imports_;
  delete functions_;
  delete statements_;
}

CollectionNode* GrammarNode::GetImports() const { return imports_; }

CollectionNode* GrammarNode::GetFunctions() const { return functions_; }

CollectionNode* GrammarNode::GetStatements() const { return statements_; }

void GrammarNode::Accept(AstWalker* walker) { walker->Visit(this); }

}  // namespace thrax
