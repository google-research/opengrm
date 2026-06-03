// Copyright 2026 The OpenGrm Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef OPENGRM_STRING_STRINGFILE_H_
#define OPENGRM_STRING_STRINGFILE_H_

#include <cstddef>
#include <string>
#include <vector>

#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "openfst/lib/file-util.h"

namespace fst {
namespace internal {

// Basic line-by-line file iterator, with support for line numbers and
// \# comment stripping.
class StringFile {
 public:
  // Opens a file input stream using the provided filename.
  explicit StringFile(absl::string_view source)
      : linenum_(0), source_(source), istrm_(source_) {
    Next();
  }

  void Reset();

  void Next();

  bool Done() const { return !istrm_; }

  absl::string_view GetString() const { return line_; }

  size_t LineNumber() const { return linenum_; }

  absl::string_view Filename() const { return source_; }

  bool Error() const { return !istrm_.is_open() || istrm_.bad(); }

 private:
  std::string line_;
  size_t linenum_;
  const std::string source_;
  file::FileInStream istrm_;
};

// File iterator expecting multiple columns separated by tab.
class ColumnStringFile {
 public:
  explicit ColumnStringFile(absl::string_view source) : sf_(source) { Parse(); }

  void Reset();

  void Next();

  bool Done() const { return sf_.Done(); }

  // Access to the underlying row vector.
  absl::Span<const absl::string_view> Row() const { return row_; }

  size_t LineNumber() const { return sf_.LineNumber(); }

  absl::string_view Filename() const { return sf_.Filename(); }

  bool Error() const { return sf_.Error(); }

 private:
  void Parse() { row_ = absl::StrSplit(sf_.GetString(), '\t'); }

  StringFile sf_;
  std::vector<absl::string_view> row_;
};

}  // namespace internal
}  // namespace fst

#endif  // OPENGRM_STRING_STRINGFILE_H_
