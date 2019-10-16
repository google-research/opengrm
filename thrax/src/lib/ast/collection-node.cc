#include <thrax/collection-node.h>

#include <deque>

#include <thrax/walker.h>
#include <thrax/compat/stlfunctions.h>

namespace thrax {

CollectionNode::CollectionNode()
    : Node(), collection_(new Collection()) {}

CollectionNode::~CollectionNode() {
  if (collection_) {
    STLDeleteContainerPointers(collection_->begin(), collection_->end());
    delete collection_;
  }
}

void CollectionNode::AddFront(Node* node) { collection_->push_front(node); }

int CollectionNode::Size() const { return collection_->size(); }

Node* CollectionNode::Get(int index) const { return (*collection_)[index]; }

Node* CollectionNode::operator[](int index) const { return Get(index); }

void CollectionNode::Accept(AstWalker* walker) { walker->Visit(this); }

}  // namespace thrax
