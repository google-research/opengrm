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

#include "opengrm/thrax/compat/file.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <ios>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "openfst/compat/compat_memory.h"

// For Cygwin and other installations that do not define ACCESSPERMS (thanks to
// Damir Cavar).
#ifndef ACCESSPERMS
#define ACCESSPERMS (S_IRWXU | S_IRWXG | S_IRWXO)
#endif

namespace thrax {

bool Readable(absl::string_view filename) {
  const int fdes = open(std::string(filename).c_str(), O_RDONLY);
  if (fdes == -1) return false;
  close(fdes);
  return true;
}

absl::Status ReadFileToString(absl::string_view file, std::string* store) {
  std::ifstream istrm{std::string(file)};
  if (!istrm) {
    if (file.empty()) {
      return absl::InternalError("No file specified for reading");
    } else {
      return absl::UnavailableError(
          absl::StrCat("Can't open file \"", file, "\" for reading"));
    }
  }
  istrm.seekg(0, std::ios::end);
  const size_t length = istrm.tellg();
  istrm.seekg(0, std::ios::beg);
  auto buf = ::fst::make_unique_for_overwrite<char[]>(length);
  istrm.read(buf.get(), length);
  store->append(buf.get(), length);
  if (istrm.fail()) return absl::InternalError("Error reading from file");
  return absl::OkStatus();
}

// A partial (largely non-) implementation of this functionality.

bool RecursivelyCreateDir(absl::string_view path) {
  if (path.empty()) return true;
  std::vector<absl::string_view> path_comp(absl::StrSplit(path, '/'));
  if (path[0] == '/') path_comp[0] = "/";  // Correct for absolute path start
  struct stat stat_buf;
  std::string rpath;
  for (auto it = path_comp.begin(); it != path_comp.end(); ++it) {
    if (it->empty() && it != path_comp.begin()) continue;
    rpath = rpath.empty() ? std::string(*it) : absl::StrCat(rpath, "/", *it);
    const int statval = stat(rpath.c_str(), &stat_buf);
    if (statval == 0) {
      if (S_ISDIR(stat_buf.st_mode)) continue;
      return false;
    } else {
      if (mkdir(rpath.c_str(), ACCESSPERMS) == -1) return false;
    }
  }
  return true;
}

File* Open(absl::string_view filename, absl::string_view mode) {
  auto m = static_cast<std::ios_base::openmode>(0);
  if (absl::StrContains(mode, 'r')) m |= std::ios::in;
  if (absl::StrContains(mode, 'w')) m |= std::ios::out;
  if (absl::StrContains(mode, 'a')) m |= std::ios::app;
  auto fstrm = std::make_unique<std::fstream>(std::string(filename).c_str(), m);
  return fstrm->fail() ? nullptr : new File(std::move(fstrm));
}

File* OpenOrDie(absl::string_view filename, absl::string_view mode) {
  auto* file = Open(filename, mode);
  if (!file) {
    if (filename.empty()) {
      LOG(FATAL) << "No file specified";
    } else {
      LOG(FATAL) << "Can't open file " << filename << " for reading";
    }
  }
  return file;
}

}  // namespace thrax
