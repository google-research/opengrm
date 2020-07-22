#ifndef FST_UTIL_STRING_STRINGFILE_H_
#define FST_UTIL_STRING_STRINGFILE_H_

#include <string>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fstream>

namespace fst {
namespace internal {

// Basic line-by-line file iterator, with support for line numbers and
// \# comment stripping.
class StringFile {
 public:
  // Opens a file input stream using the provided filename.
  explicit StringFile(const std::string &source)
      : istrm_(source), linenum_(0), source_(source) {
    if (!!istrm_) Next();
  }

  void Reset();

  void Next();

  bool Done() const { return !istrm_; }

  const std::string &GetString() const { return line_; }

  size_t LineNumber() const { return linenum_; }

  const std::string &Filename() const { return source_; }

  bool Bad() const { return istrm_.bad(); }

 private:
  std::ifstream istrm_;
  std::string line_;
  size_t linenum_;
  const std::string source_;
};

// File iterator expecting multiple columns separated by tab.
class ColumnStringFile {
 public:
  explicit ColumnStringFile(const std::string &source) : sf_(source) {
    Parse();
  }

  void Reset();

  void Next();

  bool Done() const { return sf_.Done(); }

  // Access to the underlying row vector.
  const std::vector<std::string> &Row() const { return row_; }

  size_t LineNumber() const { return sf_.LineNumber(); }

  const std::string &Filename() const { return sf_.Filename(); }

  bool Bad() const { return sf_.Bad(); }

 private:
  void Parse() { row_ = ::fst::StringSplit(sf_.GetString(), '\t'); }

  StringFile sf_;
  std::vector<std::string> row_;
};

}  // namespace internal
}  // namespace fst

#endif  // FST_UTIL_STRING_STRINGFILE_H_
