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

// The GrmManager holds a set of FSTs in memory and performs rewrites via
// composition as well as various I/O functions. GrmManager is
// thread-compatible.

#ifndef OPENGRM_THRAX_GRM_MANAGER_H_
#define OPENGRM_THRAX_GRM_MANAGER_H_

#include <memory>
#include <string>

#include "opengrm/compat/file.h"
#include "openfst/compat/file_path.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "openfst/extensions/far/far.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/symbol-table.h"
#include "opengrm/thrax/abstract-grm-manager.h"

ABSL_DECLARE_FLAG(std::string, outdir);

namespace thrax {

template <typename Arc>
class GrmManagerSpec : public AbstractGrmManager<Arc> {
  using Base = AbstractGrmManager<Arc>;

 public:
  using Base::GetFst;
  using typename Base::FstMap;

  GrmManagerSpec() : Base() {}

  ~GrmManagerSpec() override = default;

  // Loads FSTs from a FAR file, returning a non-OK status on failure.
  absl::Status LoadArchiveWithStatus(absl::string_view filename);

  // Loads FSTs from a FAR file, returning true on success.
  [[deprecated("Use LoadArchiveWithStatus instead.")]]
  bool LoadArchive(const std::string& filename);

  // Writes created FSTs into an FST archive with the provided filename.
  void ExportFar(absl::string_view filename) const override;

  // Returns the generated symbol table.
  std::unique_ptr<::fst::SymbolTable> GetGeneratedSymbolTable() const;

 private:
  GrmManagerSpec(const GrmManagerSpec&) = delete;
  GrmManagerSpec& operator=(const GrmManagerSpec&) = delete;
};

template <typename Arc>
absl::Status GrmManagerSpec<Arc>::LoadArchiveWithStatus(
    absl::string_view filename) {
  absl::StatusOr<std::unique_ptr<::fst::FarReader<Arc>>> reader =
      ::fst::STTableFarReader<Arc>::OpenWithStatus(filename);
  if (!reader.ok()) return reader.status();
  return Base::LoadArchiveWithStatus(**reader, filename);
}

template <typename Arc>
bool GrmManagerSpec<Arc>::LoadArchive(const std::string& filename) {
  absl::Status status = LoadArchiveWithStatus(filename);
  if (!status.ok()) {
    LOG(ERROR) << "Unable to open FAR: " << filename << ": " << status;
    return false;
  }
  return true;
}

template <typename Arc>
void GrmManagerSpec<Arc>::ExportFar(absl::string_view filename) const {
  const std::string dir(
      fst::JoinPath(absl::GetFlag(FLAGS_outdir), fst::Dirname(filename)));
  VLOG(1) << "Creating output directory: " << dir;
  if (!file::RecursivelyCreateDir(dir))
    LOG(FATAL) << "Unable to create output directory: " << dir;

  const std::string out_path(
      fst::JoinPath(absl::GetFlag(FLAGS_outdir), filename));
  std::unique_ptr<::fst::FarWriter<Arc>> writer(
      ::fst::STTableFarWriter<Arc>::Create(out_path));
  if (!writer) {
    LOG(FATAL) << "Failed to create writer for: " << out_path;
  }
  const auto& fsts = Base::GetFstMap();
  for (auto it = fsts.cbegin(); it != fsts.cend(); ++it) {
    VLOG(1) << "Writing FST: " << it->first;
    writer->Add(it->first, *it->second);
  }
}

template <typename Arc>
std::unique_ptr<::fst::SymbolTable>
GrmManagerSpec<Arc>::GetGeneratedSymbolTable() const {
  const auto* symbolfst = GetFst("*StringFstSymbolTable");
  return symbolfst ? absl::WrapUnique(symbolfst->InputSymbols()->Copy())
                   : nullptr;
}

using GrmManager = GrmManagerSpec<::fst::StdArc>;

}  // namespace thrax

#endif  // OPENGRM_THRAX_GRM_MANAGER_H_
