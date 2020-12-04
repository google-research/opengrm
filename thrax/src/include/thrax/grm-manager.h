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
// The GrmManager holds a set of FSTs in memory and performs rewrites via
// composition as well as various I/O functions. GrmManager is
// thread-compatible.

#ifndef NLP_GRM_LANGUAGE_GRM_MANAGER_H_
#define NLP_GRM_LANGUAGE_GRM_MANAGER_H_

#include <memory>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/compat/utils.h>
#include <fst/extensions/far/far.h>
#include <thrax/abstract-grm-manager.h>

DECLARE_string(outdir);  // From util/flags.cc.

namespace thrax {

template <typename Arc>
class GrmManagerSpec : public AbstractGrmManager<Arc> {
  using Base = AbstractGrmManager<Arc>;

 public:
  using typename Base::FstMap;

  GrmManagerSpec() : Base() {}

  ~GrmManagerSpec() override {}

  // Loads up the FSTs from a FAR file. Returns true on success and false
  // otherwise.
  bool LoadArchive(const std::string &filename);

  // This function will write the created FSTs into an FST archive with the
  // provided filename.
  void ExportFar(const std::string &filename) const override;

 private:
  GrmManagerSpec(const GrmManagerSpec &) = delete;
  GrmManagerSpec &operator=(const GrmManagerSpec &) = delete;
};

template <typename Arc>
bool GrmManagerSpec<Arc>::LoadArchive(const std::string &filename) {
  std::unique_ptr<::fst::FarReader<Arc>> reader(
#ifndef NO_GOOGLE
      ::fst::STTableFarReader<Arc>::Open(filename));
#else
      ::fst::STTableFarReader<Arc>::Open(filename));
#endif  // NO_GOOGLE
  if (!reader) {
    LOG(ERROR) << "Unable to open FAR: " << filename;
    return false;
  }
  return Base::LoadArchive(reader.get());
}

template <typename Arc>
void GrmManagerSpec<Arc>::ExportFar(const std::string &filename) const {
  const std::string dir(JoinPath(FLAGS_outdir, StripBasename(filename)));
  VLOG(1) << "Creating output directory: " << dir;
  if (!RecursivelyCreateDir(dir))
    LOG(FATAL) << "Unable to create output directory: " << dir;

  const std::string out_path(JoinPath(FLAGS_outdir, filename));
  std::unique_ptr<::fst::FarWriter<Arc>> writer(
#ifndef NO_GOOGLE
      ::fst::STTableFarWriter<Arc>::Create(out_path));
#else
      ::fst::STTableFarWriter<Arc>::Create(out_path));
#endif  // NO_GOOGLE
  if (!writer) {
    LOG(FATAL) << "Failed to create writer for: " << out_path;
  }
  const auto &fsts = Base::GetFstMap();
  for (auto it = fsts.cbegin(); it != fsts.cend(); ++it) {
    VLOG(1) << "Writing FST: " << it->first;
    writer->Add(it->first, *it->second);
  }
}

// A lot of code outside this build uses GrmManager with the old meaning of
// GrmManagerSpec<::fst::StdArc>, forward-declaring it as a class. To
// obviate the need to change all that outside code, we provide this derived
// class:

class GrmManager : public GrmManagerSpec<::fst::StdArc> {};

}  // namespace thrax

#endif  // NLP_GRM_LANGUAGE_GRM_MANAGER_H_
