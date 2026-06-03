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

// Generates a context set of a given size from an input LM.

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/fst.h"
#include "opengrm/ngram/ngram-context.h"
#include "opengrm/ngram/ngram-model.h"

ABSL_DECLARE_FLAG(int64_t, contexts);
ABSL_DECLARE_FLAG(double, bigram_threshold);

int ngramcontext_main(int argc, char** argv) {
  std::string usage = "Generates a context set from an input LM.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst] [out.fst]\n";
  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  if (argc < 2 || argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  std::string in_name = argv[1];
  std::string out_name = argc > 2 ? argv[2] : "";

  std::unique_ptr<fst::StdFst> in_fst(fst::StdFst::Read(in_name));
  if (!in_fst) return 1;

  ngram::NGramModel<fst::StdArc> ngram(*in_fst, 0, ngram::kNormEps, true);
  std::vector<std::string> contexts;
  ngram::NGramContext::FindContexts(ngram, absl::GetFlag(FLAGS_contexts),
                                    &contexts,
                                    absl::GetFlag(FLAGS_bigram_threshold));
  bool ret = ngram::NGramWriteContexts(out_name, contexts);

  return !ret;
}
