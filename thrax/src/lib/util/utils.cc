// Copyright 2005-2020 Google LLC
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
//
#include <thrax/compat/utils.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <numeric>
#include <string>
#include <vector>

#include <fst/compat.h>
#include <fst/log.h>

// For Cygwin and other installations that do not define ACCESSPERMS (thanks to
// Damir Cavar).
#ifndef ACCESSPERMS
#define ACCESSPERMS (S_IRWXU|S_IRWXG|S_IRWXO)
#endif

namespace thrax {

// Operations on filenames.

std::string JoinPath(const std::string &dirname, const std::string &basename) {
  if ((!basename.empty() && basename[0] == '/') || dirname.empty()) {
    return basename;
  } else if (dirname[dirname.size() - 1] == '/') {
    return dirname + basename;
  } else {
    return dirname + "/" + basename;
  }
}

const char *Suffix(const char *filename) {
  const char *base = strrchr(filename, '/');
  if (!base) base = filename;
  const char *last_dot = strrchr(base, '.');
  return (last_dot ? last_dot + 1 : nullptr);
}

const std::string Suffix(const std::string &filename) {
  return std::string(Suffix(filename.c_str()));
}

std::string StripBasename(const char *filename) {
  const char *base = strrchr(filename, '/');
  if (!base) return (std::string(""));
  std::string sfilename(filename);
  return sfilename.substr(0, base - filename);
}

std::string StripBasename(const std::string &filename) {
  return StripBasename(filename.c_str());
}

bool Readable(const std::string &filename) {
  const int fdes = open(filename.c_str(), O_RDONLY);
  if (fdes == -1) return false;
  close(fdes);
  return true;
}

void ReadFileToStringOrDie(const std::string &file, std::string *store) {
  std::ifstream istrm(file);
  if (!istrm) {
    if (file.empty()) {
      LOG(FATAL) << "No file specified for reading";
    } else {
      LOG(FATAL) << "Can't open file " << file << " for reading";
    }
  }
  istrm.seekg(0, std::ios::end);
  const size_t length = istrm.tellg();
  istrm.seekg(0, std::ios::beg);
  auto buf = ::fst::make_unique_for_overwrite<char[]>(length);
  istrm.read(buf.get(), length);
  store->append(buf.get(), length);
}

// A partial (largely non-) implementation of this functionality.

bool RecursivelyCreateDir(const std::string &path) {
  if (path.empty()) return true;
  std::vector<std::string> path_comp(::fst::StrSplit(path, '/'));
  if (path[0] == '/') path_comp[0] = "/" + path_comp[0];
  struct stat stat_buf;
  std::string rpath;
  for (auto it = path_comp.begin(); it != path_comp.end(); ++it) {
    rpath = rpath.empty() ? *it : rpath + "/" + *it;
    const char *crpath = rpath.c_str();
    const int statval = stat(crpath, &stat_buf);
    if (statval == 0) {
      if (S_ISDIR(stat_buf.st_mode)) continue;
      return false;
    } else {
      if (mkdir(crpath, ACCESSPERMS) == -1) return false;
    }
  }
  return true;
}

File *Open(const std::string &filename, const std::string &mode) {
  auto m = static_cast<std::ios_base::openmode>(0);
  if (mode.find('r') != std::string::npos) m |= std::ios::in;
  if (mode.find('w') != std::string::npos) m |= std::ios::out;
  if (mode.find('a') != std::string::npos) m |= std::ios::app;
  auto fstrm = std::make_unique<std::fstream>(filename.c_str(), m);
  return fstrm->fail() ? nullptr : new File(std::move(fstrm));
}

File *OpenOrDie(const std::string &filename, const std::string &mode) {
  auto *file = Open(filename, mode);
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
