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

// Trains Baum-Welch model.

#include <cstdint>

#include "absl/flags/flag.h"
#include "openfst/lib/weight.h"
#include "opengrm/baumwelch/train.h"

ABSL_FLAG(int32_t, batch_size, 0,
          "Batch size; 0 indicates full-batch training");
ABSL_FLAG(double, delta, ::fst::kDelta, "Comparison/quantization delta");
ABSL_FLAG(double, alpha, ::fst::kAlpha,
          "Step size reduction power parameter; 0 disables reduction");
ABSL_FLAG(int32_t, max_iters, ::fst::kMaxIters,
          "Maximum number of iterations to perform");
ABSL_FLAG(bool, normalize_ilabel, true,
          "Should ilabel condition normalization?");

int baumwelchtrain_main(int argc, char** argv);

int main(int argc, char** argv) { return baumwelchtrain_main(argc, argv); }
