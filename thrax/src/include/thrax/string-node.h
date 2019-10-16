// Basic node wrapper around a text string.

#ifndef THRAX_STRING_NODE_H_
#define THRAX_STRING_NODE_H_

#include <string>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/node.h>

namespace thrax {

class AstWalker;

class StringNode : public Node {
 public:
  explicit StringNode(const std::string& str);

  virtual ~StringNode();

  const std::string& Get() const;

  virtual void Accept(AstWalker* walker);

 private:
  std::string str_;

  StringNode(const StringNode&) = delete;
  StringNode& operator=(const StringNode&) = delete;
};

}  // namespace thrax

#endif  // THRAX_STRING_NODE_H_
