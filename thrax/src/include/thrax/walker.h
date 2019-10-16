#ifndef THRAX_WALKER_H_
#define THRAX_WALKER_H_

#include <fst/compat.h>
#include <thrax/compat/compat.h>
namespace thrax {

class CollectionNode;
class FstNode;
class FunctionNode;
class GrammarNode;
class IdentifierNode;
class ImportNode;
class RepetitionFstNode;
class ReturnNode;
class RuleNode;
class StatementNode;
class StringFstNode;
class StringNode;

class AstWalker {
 public:
  AstWalker();
  virtual ~AstWalker();

  virtual void Visit(CollectionNode* node) = 0;
  virtual void Visit(FstNode* node) = 0;
  virtual void Visit(FunctionNode* node) = 0;
  virtual void Visit(GrammarNode* node) = 0;
  virtual void Visit(IdentifierNode* node) = 0;
  virtual void Visit(ImportNode* node) = 0;
  virtual void Visit(RepetitionFstNode* node) = 0;
  virtual void Visit(ReturnNode* node) = 0;
  virtual void Visit(RuleNode* node) = 0;
  virtual void Visit(StatementNode* node) = 0;
  virtual void Visit(StringFstNode* node) = 0;
  virtual void Visit(StringNode* node) = 0;

 private:
  AstWalker(const AstWalker&) = delete;
  AstWalker& operator=(const AstWalker&) = delete;
};

}  // namespace thrax

#endif  // THRAX_WALKER_H_
