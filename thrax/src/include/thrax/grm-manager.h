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
//
// The GrmManager holds a set of FSTs in memory and performs rewrites via
// composition as well as various I/O functions.  GrmManager is
// thread-compatible.

#ifndef NLP_GRM_LANGUAGE_GRM_MANAGER_H_
#define NLP_GRM_LANGUAGE_GRM_MANAGER_H_

#include <map>
#include <string>
#include <vector>
using std::vector;

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/compat/utils.h>
#include <fst/extensions/far/far.h>
#include <fst/arc.h>
#include <fst/fstlib.h>
#include <fst/fst.h>
#include <fst/string.h>
#include <fst/vector-fst.h>
#include <thrax/compat/stlfunctions.h>

DECLARE_string(outdir);  // From util/flags.cc.

namespace thrax {

template <typename Arc>
class GrmManagerSpec {
 public:
  typedef fst::Fst<Arc> Transducer;
  typedef fst::VectorFst<Arc> MutableTransducer;

  GrmManagerSpec();
  ~GrmManagerSpec();

  // Loads up the FSTs from a FAR file.  Returns true on success and false
  // otherwise.
  bool LoadArchive(const string& filename);

  // Provides a pointer to the underlying FST map so that we can set it
  // directly.  Note that use of this pointer may break the thread-compatibility
  // of this class (if one thread mucks with this pointer while another thread
  // calls other const functions).
  map<string, Transducer*>* GetFstMap() { return &fsts_; }

  // ***************************************************************************
  // REWRITE: These functions perform the actual rewriting of inputs using the
  // named FSTs.

  // Rewrites the input to the output (in various parse modes).  Returns false
  // on a failed rewrite (i.e., the input was not accepted by the rule's FST)
  // and true otherwise.  Notably, a return value of true and an empty string
  // output is a deliberate rewrite to the empty string and not a failed match.
  bool RewriteBytes(const string& rule, const string& input,
                    string* output) const;
  bool RewriteBytes(const string& rule, const Transducer& input,
                    string* output) const;
  bool Rewrite(const string& rule, const string& input,
               MutableTransducer* output) const;
  bool Rewrite(const string& rule, const Transducer& input,
               MutableTransducer* output) const;

  // This helper function (when given a potential string fst) takes the shortest
  // path, projects the output, and then removes epsilon arcs.
  static void StringifyFst(MutableTransducer* output);

  // ***************************************************************************
  // The following functions give access to, modify, or serialize internal data.

  // Returns the FST associated with the particular name.  This class returns
  // the actual pointer to the FST (or NULL if it is not found), so the caller
  // should not free the pointer.
  Transducer* GetFst(const string& name) const;

  // This function will write the created FSTs into an FST archive with the
  // provided filename.
  void ExportFar(const string& filename) const;

 private:
  // Gets the named FST, just like GetFst(), but this function doesn't lock
  // anything and is thread unsafe.
  Transducer* GetFstUnlocked(const string& name) const;

  // The list of FSTs held by this manager.  It's mutable as some of the
  // Rewrite() functions might make cosmetic changes to the underlying FSTs.
  mutable map<string, Transducer*> fsts_;
  mutable fst::Mutex fsts_mutex_;

  DISALLOW_COPY_AND_ASSIGN(GrmManagerSpec);
};

template <typename Arc>
GrmManagerSpec<Arc>::GrmManagerSpec() {}

template <typename Arc>
GrmManagerSpec<Arc>::~GrmManagerSpec() {
  STLDeleteContainerPairSecondPointers(fsts_.begin(), fsts_.end());
}

template <typename Arc>
void GrmManagerSpec<Arc>::ExportFar(const string &filename) const {
  string dir = JoinPath(FLAGS_outdir, StripBasename(filename));
  VLOG(1) << "Creating output directory: " << dir;
  RecursiveCreateOptions file_create_options;
  CHECK(RecursivelyCreateDirWithOptions(dir, file_create_options))
      ;
  fst::STTableFarWriter<Arc>* writer =
      fst::STTableFarWriter<Arc>::Create(JoinPath(FLAGS_outdir,
                                                            filename));
  if (!writer)
    LOG(FATAL) << "writer must be non NULL";

  for (typename map<string, Transducer*>::const_iterator it = fsts_.begin();
       it != fsts_.end(); ++it) {
    VLOG(1) << "Writing FST: " << it->first;
    writer->Add(it->first, *it->second);
  }

  delete writer;
}

template <typename Arc>
bool GrmManagerSpec<Arc>::LoadArchive(const string& filename) {
  fst::FarReader<Arc>* reader =
      fst::STTableFarReader<Arc>::Open(filename);
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

template <typename Arc>
typename GrmManagerSpec<Arc>::Transducer*
GrmManagerSpec<Arc>::GetFst(const string& name) const {
  fst::MutexLock lock(&fsts_mutex_);
  return GetFstUnlocked(name);
}

template <typename Arc>
typename GrmManagerSpec<Arc>::Transducer*
GrmManagerSpec<Arc>::GetFstUnlocked(const string& name) const {
  Transducer* const* where = FindOrNull(fsts_, name);
  return where ? *where : NULL;
}

template <typename Arc>
bool GrmManagerSpec<Arc>::RewriteBytes(const string& rule, const string& input,
                                       string* output) const {
  fst::StringCompiler<Arc>
      string_compiler(fst::StringCompiler<Arc>::BYTE);
  MutableTransducer str_fst;
  if (!string_compiler(input, &str_fst))
    return false;
  return RewriteBytes(rule, str_fst, output);
}

template <typename Arc>
bool GrmManagerSpec<Arc>::RewriteBytes(const string& rule,
                                       const Transducer& input,
                                       string* output) const {
  MutableTransducer output_fst;
  if (!Rewrite(rule, input, &output_fst))
    return false;

  StringifyFst(&output_fst);

  fst::StringPrinter<Arc> printer(fst::StringPrinter<Arc>::BYTE);
  return printer(output_fst, output);
}

template <typename Arc>
bool GrmManagerSpec<Arc>::Rewrite(const string& rule, const string& input,
                                  MutableTransducer* output) const {
  fst::StringCompiler<Arc>
      string_compiler(fst::StringCompiler<Arc>::BYTE);
  MutableTransducer str_fst;
  if (!string_compiler(input, &str_fst))
    return false;

  return Rewrite(rule, str_fst, output);
}

template <typename Arc>
bool GrmManagerSpec<Arc>::Rewrite(const string& rule, const Transducer& input,
                                  MutableTransducer* output) const {

  Transducer* rule_fst = GetFstUnlocked(rule);
  if (!rule_fst) {
    LOG(ERROR) << "Rule " << rule << " not found.";
    return false;
  }

  // If the rule FST isn't input-label sorted, do it now so we can compose with
  // all inputs.  This will also carry through to future rewrites.
  if (!rule_fst->Properties(fst::kILabelSorted, false)) {
    VLOG(1) << "Updating FST " << rule << " with input label sorted version.";

    MutableTransducer* sorted_rule_fst = new MutableTransducer(*rule_fst);
    fst::ArcSort(sorted_rule_fst, fst::ILabelCompare<Arc>());

    delete rule_fst;
    rule_fst = static_cast<Transducer*>(sorted_rule_fst);
    fsts_[rule] = rule_fst;
  }

  fst::ComposeFst<Arc> cfst(input, *rule_fst);
  *output = cfst;
  fst::Connect(output);

  return true;
}

template <typename Arc>
void GrmManagerSpec<Arc>::StringifyFst(MutableTransducer* fst) {
  MutableTransducer temp;
  fst::ShortestPath(*fst, &temp);
  fst::Project(&temp, fst::PROJECT_OUTPUT);
  fst::RmEpsilon(&temp);
  *fst = temp;
}

// A lot of code outside this build uses GrmManager with the old meaning of
// GrmManagerSpec<fst::StdArc>, forward-declaring it as a class. To obviate
// the need to change all that outside code, we provide this derived class:

class GrmManager : public GrmManagerSpec<fst::StdArc> {};

}  // namespace thrax

#endif  // NLP_GRM_LANGUAGE_GRM_MANAGER_H_
