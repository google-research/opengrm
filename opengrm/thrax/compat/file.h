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

// File I/O APIs.

#ifndef OPENGRM_THRAX_COMPAT_FILE_H_
#define OPENGRM_THRAX_COMPAT_FILE_H_

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <memory>
#include <string>
#include <utility>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"

namespace thrax {

bool Readable(absl::string_view filename);

absl::Status ReadFileToString(absl::string_view filename, std::string* store);

bool RecursivelyCreateDir(absl::string_view path);

class File {
 public:
  File() {}

  explicit File(std::fstream* stream) : stream_(stream) {}

  explicit File(std::unique_ptr<std::fstream>&& stream)
      : stream_(std::move(stream)) {}

  void SetStream(std::fstream* stream) { stream_.reset(stream); }

  void SetStream(std::unique_ptr<std::fstream>&& stream) {
    stream_ = std::move(stream);
  }

  std::fstream* Stream() { return stream_.get(); }

  void Close() { stream_.reset(); }

 private:
  std::unique_ptr<std::fstream> stream_;
};

class InputBuffer {
 public:
  // 2^14 should be enough for 1 line for the intended use.
  constexpr static int kMaxLine = 16384;

  explicit InputBuffer(File* fp) : fp_(fp) {}

  bool ReadLine(std::string* line) {
    line->clear();
    fp_->Stream()->getline(buf_, kMaxLine);
    if (!fp_->Stream()->gcount()) {
      fp_.reset();
      return false;
    }
    line->append(buf_);
    return true;
  }

  bool CloseFile() {
    if (fp_.get()) fp_->Close();
    return true;
  }

 private:
  std::unique_ptr<File> fp_;
  char buf_[kMaxLine];
};

File* Open(absl::string_view filename, absl::string_view mode);

File* OpenOrDie(absl::string_view filename, absl::string_view mode);

}  // namespace thrax

#endif  // OPENGRM_THRAX_COMPAT_FILE_H_
