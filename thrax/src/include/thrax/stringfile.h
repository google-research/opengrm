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
// Author: rws@google.com (Richard Sproat)
//
// Reads in a file of strings to be compiled into an fst
// uses the speech/fst/prefix_tree functionality.

#ifndef THRAX_STRINGFILE_H_
#define THRAX_STRINGFILE_H_

#include <string>
#include <vector>
using std::vector;

#include <thrax/compat/utils.h>
#include <fst/arcsort.h>
#include <fst/fst.h>
#include <fst/project.h>
#include <fst/rmepsilon.h>
#include <fst/vector-fst.h>
#include <thrax/datatype.h>
#include <thrax/function.h>
#include <thrax/algo/prefix_tree.h>
#include <thrax/compat/stlfunctions.h>

DECLARE_string(indir);  // From util/flags.cc.

namespace thrax {
namespace function {

template <typename Arc>
class StringFile : public Function<Arc> {
 public:
  typedef fst::VectorFst<Arc> MutableTransducer;
  typedef fst::PrefixTree<Arc> PrefixTree;

  StringFile() : Function<Arc>() {}
  virtual ~StringFile() {}

 protected:
  virtual DataType* Execute(const vector<DataType*>& args) {
    if (args.size() != 1) {
      cout << "StringFile: Expected 1 argument but got " << args.size() << endl;
      return NULL;
    }
    if (!args[0]->is<string>()) {
      cout << "StringFile: Expected string (file) for argument 1" << endl;
      return NULL;
    }
    const string& filename =
        JoinPath(FLAGS_indir, *args[0]->get<string>());

    File* fp = OpenOrDie(filename, "r");
    PrefixTree pt;
    string word;
    for (InputBuffer ibuf(fp); ibuf.ReadLine(&word);
         /* ReadLine() automatically increments */) {
      if (word.empty())
        continue;

      // Add to the prefix tree the full word (line) as the input going to the
      // empty string as the output.  We'll use identical pointers (iterators)
      // to show the empty string.
      pt.Add(word.begin(), word.end(),     // Input = word.
             word.begin(), word.begin());  // Output = empty string ("").
    }

    MutableTransducer* fst = new MutableTransducer();
    pt.ToFst(fst);
    fst::Project(fst, fst::PROJECT_INPUT);
    fst::RmEpsilon(fst);
    fst::ArcSort(fst, arcsort_comparer_);

    return new DataType(fst);
  }

 private:
  static const fst::ILabelCompare<Arc> arcsort_comparer_;

  DISALLOW_COPY_AND_ASSIGN(StringFile<Arc>);
};

template <typename Arc>
const fst::ILabelCompare<Arc> StringFile<Arc>::arcsort_comparer_ =
    fst::ILabelCompare<Arc>();

}  // namespace function
}  // namespace thrax

#endif  // THRAX_STRINGFILE_H_
