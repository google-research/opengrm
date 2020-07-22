#include <thrax/algo/stringfile.h>

#include <thrax/algo/stringutil.h>

namespace fst {
namespace internal {

void StringFile::Reset() {
  istrm_.clear();
  istrm_.seekg(0, istrm_.beg);
  Next();
}

// Tries to read a non-empty line until EOF.
void StringFile::Next() {
  do {
    ++linenum_;
    if (!std::getline(istrm_, line_)) return;
    line_ = StripCommentAndRemoveEscape(line_);
  } while (line_.empty());
}

void ColumnStringFile::Reset() {
  sf_.Reset();
  Parse();
}

void ColumnStringFile::Next() {
  sf_.Next();
  Parse();
}

}  // namespace internal
}  // namespace fst
