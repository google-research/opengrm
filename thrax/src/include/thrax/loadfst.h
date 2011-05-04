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
// Loads up an FST from the provided filename.

#ifndef THRAX_LOADFST_H_
#define THRAX_LOADFST_H_

#include <string>
#include <vector>
using std::vector;

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/compat/utils.h>
#include <fst/fst.h>
#include <thrax/function.h>
#include <thrax/datatype.h>

DECLARE_string(indir);  // From util/flags.cc.

namespace thrax {
namespace function {

template <typename Arc>
class LoadFst : public Function<Arc> {
 public:
  typedef fst::Fst<Arc> Transducer;

  LoadFst() {}
  virtual ~LoadFst() {}

 protected:
  virtual DataType* Execute(const vector<DataType*>& args) {
    if (args.size() != 1) {
      cout << "LoadFst: Expected 1 argument but got " << args.size() << endl;
      return NULL;
    }

    if (!args[0]->is<string>()) {
      cout << "LoadFst: Expected string (path) for argument 1" << endl;
      return NULL;
    }
    const string& file = JoinPath(FLAGS_indir, *args[0]->get<string>());

    VLOG(2) << "Loading FST: " << file;
    Transducer* fst = Transducer::Read(file);
    if (!fst) {
      cout << "LoadFst: Failed to load FST from file: " << file << endl;
      return NULL;
    }

    return new DataType(fst);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(LoadFst<Arc>);
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_LOADFST_H_
