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

#ifndef OPENGRM_THRAX_WALKER_UTIL_FUNCTION_TEMP_FILE_H_
#define OPENGRM_THRAX_WALKER_UTIL_FUNCTION_TEMP_FILE_H_

#include <cstdio>
#include <string>

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/file-util.h"

namespace thrax {
namespace function {

class TempFile {
 public:
  TempFile(const std::string& name, absl::string_view content)
      : path_(fst::JoinPath(::testing::TempDir(), name)) {
    file::FileOutStream out(path_);
    out << content;
  }
  ~TempFile() { std::remove(path_.c_str()); }
  const std::string& path() const { return path_; }

 private:
  std::string path_;
};

}  // namespace function
}  // namespace thrax

#endif  // OPENGRM_THRAX_WALKER_UTIL_FUNCTION_TEMP_FILE_H_
