// A statement is a generic command to be executed. Currently, we support only
// return statements and rule (assignment) statements.

#ifndef THRAX_STATEMENT_NODE_H_
#define THRAX_STATEMENT_NODE_H_

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/node.h>

namespace thrax {

class AstWalker;

class StatementNode : public Node {
 public:
  enum StatementNodeType {
    RULE_STATEMENTNODE,
    RETURN_STATEMENTNODE,
  };

  explicit StatementNode(StatementNodeType type);

  virtual ~StatementNode();

  StatementNodeType GetType() const;

  void Set(Node* statement);

  Node* Get() const;

  virtual void Accept(AstWalker* walker);

 private:
  StatementNodeType type_;
  Node* statement_;

  StatementNode(const StatementNode&) = delete;
  StatementNode& operator=(const StatementNode&) = delete;
};

}  // namespace thrax

#endif  // THRAX_STATEMENT_NODE_H_
