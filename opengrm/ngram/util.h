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

#ifndef OPENGRM_NGRAM_UTIL_H_
#define OPENGRM_NGRAM_UTIL_H_

#include "absl/base/log_severity.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"

// Utilities for error handling.

ABSL_DECLARE_FLAG(bool, ngram_error_fatal);

#define NGRAMERROR()                               \
  LOG(LEVEL(absl::GetFlag(FLAGS_ngram_error_fatal) \
                ? absl::LogSeverity::kFatal        \
                : absl::LogSeverity::kError))

#endif  // OPENGRM_NGRAM_UTIL_H_
