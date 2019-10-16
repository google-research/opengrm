// A function is a repeatable set of assignments and commands used to build a
// new FST from inputs. This node contains the name of the function along with
// the bound argument names and the body statements.

#ifndef THRAX_FUNCTION_NODE_H_
#define THRAX_FUNCTION_NODE_H_

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/node.h>

namespace thrax {

class AstWalker;
class CollectionNode;
class IdentifierNode;

class FunctionNode : public Node {
 public:
  FunctionNode(IdentifierNode* name,
               CollectionNode* arguments,
               CollectionNode* body);

  virtual ~FunctionNode();

  IdentifierNode* GetName() const;

  CollectionNode* GetArguments() const;

  CollectionNode* GetBody() const;

  virtual void Accept(AstWalker* walker);

 private:
  IdentifierNode* name_;
  CollectionNode* arguments_;
  CollectionNode* body_;

  FunctionNode(const FunctionNode&) = delete;
  FunctionNode& operator=(const FunctionNode&) = delete;
};

}  // namespace thrax

#endif  // THRAX_FUNCTION_NODE_H_
