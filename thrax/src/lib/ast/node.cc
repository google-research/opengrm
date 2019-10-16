#include <thrax/node.h>

namespace thrax {

Node::Node() : line_number_(-1) {}

Node::~Node() {}

void Node::SetLine(int line) { line_number_ = line; }

int Node::getline() const { return line_number_; }

}  // namespace thrax
