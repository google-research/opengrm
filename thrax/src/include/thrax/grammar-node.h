// The main grammar is the top node (i.e., root) of the AST. A grammar consists
// of a list of imports, a list of functions, and a list of body functions.

#ifndef THRAX_GRAMMAR_NODE_H_
#define THRAX_GRAMMAR_NODE_H_

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/node.h>

namespace thrax {

class AstWalker;
class CollectionNode;

class GrammarNode : public Node {
 public:
  GrammarNode(CollectionNode* imports,
              CollectionNode* functions,
              CollectionNode* statements);

  virtual ~GrammarNode();

  CollectionNode* GetImports() const;

  CollectionNode* GetFunctions() const;

  CollectionNode* GetStatements() const;

  virtual void Accept(AstWalker* walker);

 private:
  CollectionNode* imports_;
  CollectionNode* functions_;
  CollectionNode* statements_;

  GrammarNode(const GrammarNode&) = delete;
  GrammarNode& operator=(const GrammarNode&) = delete;
};

}  // namespace thrax

#endif  // THRAX_GRAMMAR_NODE_H_
