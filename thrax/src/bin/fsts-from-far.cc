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
// From the given FAR, we'll extract the specified FSTs and write them out to
// the provided output directory, using filenames composed of the FST key
// combined with the desired extension (".fst" by default).
//
// This does pretty much what nlp/fst/extensions/far:farextract does, but with a
// much simpler syntax and faster execution time, and friendlier flags for BUILD
// extension rule writing.  The flags are more straightforward, avoiding the
// prefix/suffix and arbitrary-numbering scheme.  Technically, this calls on the
// FAR library directly to pull the FSTs, instead of going through the FstScript
// libraries, resulting in a faster binary.

#include <string>
#include <vector>
using std::vector;

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/compat/utils.h>
#include <fst/extensions/far/far.h>
#include <fst/arc.h>
#include <fst/fst.h>
#include <thrax/compat/utils.h>

DEFINE_string(input_far, "", "Path to the FAR.");
DEFINE_string(fst_names, "", "List of keys of the FST to extract from the FAR, "
              "comma-delimited.");
DEFINE_string(output_path, "", "Output directory for the written FSTs.");
DEFINE_string(fst_extension, ".fst", "Suffix attached to output filenames.");

using fst::FarReader;
using fst::Fst;
typedef fst::StdArc Arc;
using thrax::SplitStringUsing;
using thrax::JoinPath;

int main(int argc, char** argv) {
  std::set_new_handler(FailedNewHandler);
  SetFlags(argv[0], &argc, &argv, true);

  FarReader<Arc>* reader = FarReader<Arc>::Open(FLAGS_input_far);
  CHECK(reader != NULL)
      ;

  vector<string> fst_names;
  SplitStringUsing(FLAGS_fst_names, ",", &fst_names);

  for (int i = 0; i < fst_names.size(); ++i) {
    CHECK(reader->Find(fst_names[i]))
        ;

    const Fst<Arc>& fst = reader->GetFst();
    string fst_path = JoinPath(FLAGS_output_path,
                                     fst_names[i] + FLAGS_fst_extension);
    CHECK(fst.Write(fst_path))
        ;

    VLOG(1) << "Extracted FST " << fst_names[i] << " from " << FLAGS_input_far
            << " to " << fst_path << ".";
  }

  delete reader;
  return 0;
}
