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
//
// Simple implementation of StrCat, needed in various places. This version
// allows from 2 to 5 combinations of strings and ints.

#ifndef THRAX_COMPAT_STRUTILS_H_
#define THRAX_COMPAT_STRUTILS_H_

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdarg>
#include <cstdio>

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <fst/compat.h>

namespace thrax {

// Operations on strings.

class StringOrInt {
 public:
  StringOrInt(const std::string &s) : str_(s) {}  // NOLINT

  StringOrInt(const char *s) : str_(std::string(s)) {}  // NOLINT

  StringOrInt(int i) {  // NOLINT
    char buf[1024];
    sprintf(buf, "%d", i);
    str_ = std::string(buf);
  }

  const std::string &Get() const { return str_; }

 private:
  std::string str_;
};

// TODO(kbg): Make this work with variadic template, maybe.

inline std::string StringCat(const StringOrInt &s1, const StringOrInt &s2) {
  return s1.Get() + s2.Get();
}

inline std::string StringCat(const StringOrInt &s1, const StringOrInt &s2,
                             const StringOrInt &s3) {
  return s1.Get() + StringCat(s2, s3);
}

inline std::string StringCat(const StringOrInt &s1, const StringOrInt &s2,
                             const StringOrInt &s3, const StringOrInt &s4) {
  return s1.Get() + StringCat(s2, s3, s4);
}

inline std::string StringCat(const StringOrInt &s1, const StringOrInt &s2,
                             const StringOrInt &s3, const StringOrInt &s4,
                             const StringOrInt &s5) {
  return s1.Get() + StringCat(s2, s3, s4, s5);
}

inline void StringReplace(std::string *full, const std::string &before,
                          const std::string &after) {
  size_t pos = 0;
  while ((pos = full->find(before, pos)) != std::string::npos) {
    full->replace(pos, before.size(), after);
    pos += after.size();
  }
}

inline std::string StringReplace(const std::string &full,
                                 const std::string &before,
                                 const std::string &after, bool /* ignored */) {
  std::string copy(full);
  StringReplace(&copy, before, after);
  return copy;
}

std::string StringPrintf(const char *format, ...);

// Operations on filenames.

std::string JoinPath(const std::string &dirname, const std::string &basename);

const char *Suffix(const char *filename);

const std::string Suffix(const std::string &filename);

std::string StripBasename(const char *filename);

std::string StripBasename(const std::string &filename);

bool Readable(const std::string &filename);

void ReadFileToStringOrDie(const std::string &filename, std::string *store);

bool RecursivelyCreateDir(const std::string &path);

class File {
 public:
  File() {}

  explicit File(std::fstream *stream) : stream_(stream) {}

  explicit File(std::unique_ptr<std::fstream> &&stream)
      : stream_(std::move(stream)) {}

  void SetStream(std::fstream *stream) { stream_.reset(stream); }

  void SetStream(std::unique_ptr<std::fstream> &&stream) {
    stream_ = std::move(stream);
  }

  std::fstream *Stream() { return stream_.get(); }

  void Close() { stream_.reset(); }

 private:
  std::unique_ptr<std::fstream> stream_;
};

// 2^14 should be enough for 1 line for the intended use.

class InputBuffer {
 public:
  constexpr static int kMaxLine = 16384;

  explicit InputBuffer(File *fp) : fp_(fp) {}

  bool ReadLine(std::string *line) {
    line->clear();
    fp_->Stream()->getline(buf_, kMaxLine);
    if (!fp_->Stream()->gcount()) {
      fp_.reset();
      return false;
    }
    line->append(buf_);
    return true;
  }

 private:
  std::unique_ptr<File> fp_;
  char buf_[kMaxLine];
};

File *Open(const std::string &filename, const std::string &mode);

File *OpenOrDie(const std::string &filename, const std::string &mode);

}  // namespace thrax

#endif  // THRAX_COMPAT_STRUTILS_H_
