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

#include <cstdint>

#include "absl/flags/flag.h"
#include "opengrm/ngram/ngram-model.h"

ABSL_FLAG(int64_t, backoff_label, 0, "Backoff label");
ABSL_FLAG(int32_t, max_bo_updates, 10,
          "Max iterations of backoff re-calculation");
ABSL_FLAG(double, norm_eps, ngram::kNormEps, "Normalization check epsilon");
ABSL_FLAG(bool, check_consistency, false, "Check model consistency");

int ngrammarginalize_main(int argc, char** argv);
int main(int argc, char** argv) { return ngrammarginalize_main(argc, argv); }
