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
