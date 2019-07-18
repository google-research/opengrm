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
// Copyright 2017 and onwards Google, Inc.

// Trains Baum-Welch channel model.

#include <unistd.h>

#include <fst/flags.h>
#include <baumwelch/baumwelch.h>

DEFINE_bool(decipherment, false,
            "Use decipherment construction; i.e., input is a WFSA rather "
            "than a FAR");
DEFINE_string(expectation_table, "state_ilabel",
              "Expectation table, one of: \"global\", \"state\", \"ilabel\", "
              "\"state_ilabel\"");
DEFINE_int32(max_iters, fst::kMaxIters,
             "Maximum number of iterations to perform");
DEFINE_bool(flat_start, true, "Perform one round of flat start training?");
DEFINE_int32(random_starts, fst::kRandomStarts,
             "Number of random starts to perform");
DEFINE_bool(remove_zero_arcs, true, "Should zero arcs be removed?");
DEFINE_double(delta, fst::kDelta, "Comparison/quantization delta");
DEFINE_int32(seed, time(nullptr) + getpid(), "Random seed");

int baumwelchtrain_main(int argc, char **argv);

int main(int argc, char **argv) { return baumwelchtrain_main(argc, argv); }

