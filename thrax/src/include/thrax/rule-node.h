// A rule node is an assignment of an FST or other object to a variable. The
// rule node contains the left and right hand sides of the assignment.

#ifndef THRAX_RULE_NODE_H_
#define THRAX_RULE_NODE_H_

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/node.h>

namespace thrax {

class AstWalker;
class IdentifierNode;

class RuleNode : public Node {
 public:
  enum ExportStatus {
    EXPORT,
    DO_NOT_EXPORT,
  };

  RuleNode(IdentifierNode* name, Node* rhs, ExportStatus exp);

  virtual ~RuleNode();

  IdentifierNode* GetName() const;

  Node* Get() const;

  bool ShouldExport() const;

  virtual void Accept(AstWalker* walker);

 private:
  IdentifierNode* name_;
  Node* rhs_;
  ExportStatus export_;

  RuleNode(const RuleNode&) = delete;
  RuleNode& operator=(const RuleNode&) = delete;
};

}  // namespace thrax

#endif  // THRAX_RULE_NODE_H_
