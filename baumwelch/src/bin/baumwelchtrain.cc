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

// Trains Baum-Welch model.

#include <random>

#include <fst/flags.h>
#include <baumwelch/train.h>

DEFINE_int32(batch_size, 0, "Batch size; 0 indicates full-batch training");
DEFINE_double(delta, ::fst::kDelta, "Comparison/quantization delta");
DEFINE_double(lr, ::fst::kLr, "Learning rate");
DEFINE_int32(max_iters, ::fst::kMaxIters,
             "Maximum number of iterations to perform");
DEFINE_bool(normalize_ilabel, true, "Should ilabel be used in normalization?");

int baumwelchtrain_main(int argc, char **argv);

int main(int argc, char **argv) { return baumwelchtrain_main(argc, argv); }

