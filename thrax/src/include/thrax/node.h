// The generic, basic node for our AST.

#ifndef THRAX_NODE_H_
#define THRAX_NODE_H_

#include <fst/compat.h>
#include <thrax/compat/compat.h>
namespace thrax {

class AstWalker;

class Node {
 public:
  Node();

  virtual ~Node();

  virtual void Accept(AstWalker* walker) = 0;

  // Set and get the line number where this node is stored.

  void SetLine(int line);

  int getline() const;

 protected:
  int line_number_;  // Where in the file this node is found.

 private:
  Node(const Node &) = delete;
  Node &operator=(const Node &) = delete;
};

}  // namespace thrax

#endif  // THRAX_NODE_H_
