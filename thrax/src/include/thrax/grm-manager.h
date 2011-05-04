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
// composition as well as various I/O functions.

#ifndef NLP_GRM_LANGUAGE_GRM_MANAGER_H_
#define NLP_GRM_LANGUAGE_GRM_MANAGER_H_

#include <map>
#include <string>
#include <vector>
using std::vector;

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/arc.h>
#include <fst/fst.h>
#include <fst/vector-fst.h>

namespace thrax {

class GrmManager {
 public:
  typedef fst::StdArc Arc;
  typedef fst::Fst<Arc> Transducer;
  typedef fst::VectorFst<Arc> MutableTransducer;

  GrmManager();
  ~GrmManager();

  // Loads up the FSTs from a FAR file.  Returns true on success and false
  // otherwise.
  bool LoadArchive(const string& filename);

  // Provides a pointer to the underlying FST map so that we can set it
  // directly.
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
  bool Rewrite(const string& rule, const Transducer& input,
               MutableTransducer* output) const;

  // Given a potential string fst, this will take the shortest path, project the
  // output, and then remove epsilon arcs.
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
  // The list of FSTs held by this manager.  It's mutable as some of the
  // Rewrite() functions might make cosmetic changes to the underlying FSTs.
  mutable map<string, Transducer*> fsts_;

  DISALLOW_COPY_AND_ASSIGN(GrmManager);
};

}  // namespace thrax

#endif  // NLP_GRM_LANGUAGE_GRM_MANAGER_H_
