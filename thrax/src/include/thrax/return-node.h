// Return statements tell us what FST to return from a function, allowing
// binding to a variable in the main program.

#ifndef THRAX_RETURN_NODE_H_
#define THRAX_RETURN_NODE_H_

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/node.h>

namespace thrax {

class AstWalker;
class IdentifierNode;

class ReturnNode : public Node {
 public:
  explicit ReturnNode(Node* node);

  virtual ~ReturnNode();

  Node* Get() const;

  virtual void Accept(AstWalker* walker);

 private:
  Node* node_;

  ReturnNode(const ReturnNode&) = delete;
  ReturnNode& operator=(const ReturnNode&) = delete;
};

}  // namespace thrax

#endif  // THRAX_RETURN_NODE_H_
