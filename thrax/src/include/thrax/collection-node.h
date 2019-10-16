// This houses a list of other nodes. During construction, we'll create them by
// pushing onto the front of the list.

#ifndef THRAX_COLLECTION_NODE_H_
#define THRAX_COLLECTION_NODE_H_

#include <deque>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/node.h>

namespace thrax {

class AstWalker;

class CollectionNode : public Node {
 public:
  using Collection = std::deque<Node*>;

  CollectionNode();

  virtual ~CollectionNode();

  // Add a collection to the list.
  void AddFront(Node* node);

  int Size() const;

  Node* Get(int index) const;

  Node* operator[](int index) const;

  virtual void Accept(AstWalker* walker);

 private:
  Collection* collection_;

  CollectionNode(const CollectionNode&) = delete;
  CollectionNode& operator=(const CollectionNode&) = delete;
};

}  // namespace thrax

#endif  // THRAX_COLLECTION_NODE_H_
