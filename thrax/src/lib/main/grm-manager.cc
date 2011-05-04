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
// Copyright 2005-2011 Google, Inc.
// Author: ttai@google.com (Terry Tai)

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/grm-manager.h>

#include <map>
#include <string>
#include <vector>
using std::vector;

#include <thrax/compat/utils.h>
#include <fst/extensions/far/far.h>
#include <fst/arcsort.h>
#include <fst/compose.h>
#include <fst/fst.h>
#include <fst/project.h>
#include <fst/properties.h>
#include <fst/rmepsilon.h>
#include <fst/shortest-path.h>
#include <fst/string.h>
#include <thrax/compat/stlfunctions.h>

DECLARE_string(outdir);  // From util/flags.cc.

using fst::ArcSort;
using fst::ComposeFst;
using fst::FarReader;
using fst::ILabelCompare;
using fst::STTableFarReader;
using fst::STTableFarWriter;
using fst::StringCompiler;
using fst::StringPrinter;

namespace thrax {

GrmManager::GrmManager() {}

GrmManager::~GrmManager() {
  STLDeleteContainerPairSecondPointers(fsts_.begin(), fsts_.end());
}

void GrmManager::ExportFar(const string &filename) const {
  string dir = JoinPath(FLAGS_outdir, StripBasename(filename));
  VLOG(1) << "Creating output directory: " << dir;
  RecursiveCreateOptions file_create_options;
  CHECK(RecursivelyCreateDirWithOptions(dir, file_create_options))
      ;
  STTableFarWriter<Arc>* writer = STTableFarWriter<Arc>::Create(
      JoinPath(FLAGS_outdir, filename));
  if (!writer)
    LOG(FATAL) << "writer must be non NULL";

  for (map<string, Transducer*>::const_iterator it = fsts_.begin();
       it != fsts_.end(); ++it) {
    VLOG(1) << "Writing FST: " << it->first;
    writer->Add(it->first, *it->second);
  }

  delete writer;
}

bool GrmManager::LoadArchive(const string& filename) {
  FarReader<Arc>* reader = STTableFarReader<Arc>::Open(filename);
  if (!reader) {
    cout << "Unable to open FAR: " << filename;
    return false;
  }

  STLDeleteContainerPairSecondPointers(fsts_.begin(), fsts_.end());
  fsts_.clear();
  for (reader->Reset(); !reader->Done(); reader->Next()) {
    const string& name = reader->GetKey();
    Transducer* fst = new MutableTransducer(reader->GetFst());
    fsts_[name] = fst;
  }

  delete reader;
  return true;
}

GrmManager::Transducer* GrmManager::GetFst(const string& name) const {
  Transducer* const* where = FindOrNull(fsts_, name);
  return where ? *where : NULL;
}

bool GrmManager::RewriteBytes(const string& rule, const string& input,
                              string* output) const {
  StringCompiler<Arc> string_compiler(StringCompiler<Arc>::BYTE);
  MutableTransducer str_fst;
  if (!string_compiler(input, &str_fst))
    return false;
  return RewriteBytes(rule, str_fst, output);
}

bool GrmManager::RewriteBytes(const string& rule, const Transducer& input,
                              string* output) const {
  MutableTransducer output_fst;
  if (!Rewrite(rule, input, &output_fst))
    return false;

  StringifyFst(&output_fst);

  StringPrinter<Arc> printer(StringPrinter<Arc>::BYTE);
  return printer(output_fst, output);
}

bool GrmManager::Rewrite(const string& rule, const Transducer& input,
                         MutableTransducer* output) const {
  Transducer* rule_fst = GetFst(rule);
  if (!rule_fst) {
    LOG(ERROR) << "Rule " << rule << " not found.";
    return false;
  }

  // If the rule FST isn't input-label sorted, do it now so we can compose with
  // all inputs.  This will also carry through to future rewrites.
  if (!rule_fst->Properties(fst::kILabelSorted, false)) {
    MutableTransducer* sorted_rule_fst = new MutableTransducer(*rule_fst);
    ArcSort(sorted_rule_fst, ILabelCompare<Arc>());

    delete rule_fst;
    rule_fst = static_cast<Transducer*>(sorted_rule_fst);
    fsts_[rule] = rule_fst;
  }

  ComposeFst<Arc> cfst(input, *rule_fst);
  *output = cfst;

  return true;
}

void GrmManager::StringifyFst(MutableTransducer* fst) {
  MutableTransducer temp;
  fst::ShortestPath(*fst, &temp);
  fst::Project(&temp, fst::PROJECT_OUTPUT);
  fst::RmEpsilon(&temp);
  *fst = temp;
}

}  // namespace thrax
