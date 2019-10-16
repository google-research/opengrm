#ifndef THRAX_PRINTER_H_
#define THRAX_PRINTER_H_

#include <iostream>
#include <string>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/node.h>
#include <thrax/walker.h>

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

// An AST walker that prints out each node as it visits it.
class AstPrinter : public AstWalker {
 public:
  AstPrinter();
  explicit AstPrinter(std::ostream& output_stream);
  virtual ~AstPrinter();

  virtual void Visit(CollectionNode* node);
  virtual void Visit(FstNode* node);
  virtual void Visit(FunctionNode* node);
  virtual void Visit(GrammarNode* node);
  virtual void Visit(IdentifierNode* node);
  virtual void Visit(ImportNode* node);
  virtual void Visit(RepetitionFstNode* node);
  virtual void Visit(ReturnNode* node);
  virtual void Visit(RuleNode* node);
  virtual void Visit(StatementNode* node);
  virtual void Visit(StringFstNode* node);
  virtual void Visit(StringNode* node);

  // If true, put line numbers into ast.
  bool include_line_numbers = false;

 private:
  // Returns the spacing prefix for the current nested level, plus the line
  // number if line_numbers is true.
  std::string Spaces(Node* node) const;

  // A scoped counter that increments and decrements the spacing as necessary.
  class ScopedSpaceCounter {
   public:
    explicit ScopedSpaceCounter(int* num_spaces);
    ~ScopedSpaceCounter();

   private:
    int* num_spaces_;
  };

  int num_spaces_;

  mutable bool argument_;

  // This is the actual stream to which we write out, which we do not own.
  std::ostream& out;

  AstPrinter(const AstPrinter&) = delete;
  AstPrinter& operator=(const AstPrinter&) = delete;
};

}  // namespace thrax

#endif  // THRAX_PRINTER_H_
