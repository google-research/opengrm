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

#include <thrax/compat/utils.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdarg>
#include <cstdio>

#include <fstream>
#include <numeric>
#include <string>
#include <vector>

// For Cygwin and other installations that do not define ACCESSPERMS (thanks to
// Damir Cavar).
#ifndef ACCESSPERMS
#define ACCESSPERMS (S_IRWXU|S_IRWXG|S_IRWXO)
#endif

namespace thrax {

// Operations on strings.

// Split a string according to the delimiters, including consecutive
// delimiters as empty strings.
void StripSplitAllowEmpty(const std::string &full, const char *delim,
                          std::vector<std::string> *result) {
  size_t prev = 0;
  size_t found = full.find_first_of(delim);
  result->push_back(full.substr(prev, found - prev));
  while (found != std::string::npos) {
    prev = found + 1;
    found = full.find_first_of(delim, prev);
    result->push_back(full.substr(prev, found - prev));
  }
}

std::string StringPrintf(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  char buf[1024];
  vsnprintf(buf, 1024, format, ap);
  std::string retval(buf);
  va_end(ap);
  return retval;
}

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
  char *buf = new char[length];
  istrm.read(buf, length);
  store->append(buf, length);
  delete[] buf;
}

// A partial (largely non-) implementation of this functionality.

bool RecursivelyCreateDir(const std::string &path) {
  if (path.empty()) return true;
  std::vector<std::string> path_comp(::fst::StringSplit(path, "/"));
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
  auto fstrm = ::fst::make_unique<std::fstream>(filename.c_str(), m);
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
