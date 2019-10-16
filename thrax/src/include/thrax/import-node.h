// An import statement is one that loads up another module. This node contains
// the path and alias for an import.

#ifndef THRAX_IMPORT_NODE_H_
#define THRAX_IMPORT_NODE_H_

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/node.h>

namespace thrax {

class AstWalker;
class IdentifierNode;
class StringNode;

class ImportNode : public Node {
 public:
  ImportNode(StringNode* path, IdentifierNode* alias);

  virtual ~ImportNode();

  StringNode* GetPath() const;

  IdentifierNode* GetAlias() const;

  virtual void Accept(AstWalker* walker);

 private:
  StringNode* path_;
  IdentifierNode* alias_;

  ImportNode(const ImportNode&) = delete;
  ImportNode& operator=(const ImportNode&) = delete;
};

}  // namespace thrax

#endif  // THRAX_IMPORT_NODE_H_
