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

// Compiles a context-dependent rewrite rule.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "openfst/script/fst-class.h"
#include "opengrm/operators/cdrewrite.h"
#include "opengrm/operators/cdrewritescript.h"
#include "opengrm/operators/getters.h"

ABSL_DECLARE_FLAG(std::string, direction);
ABSL_DECLARE_FLAG(std::string, mode);
ABSL_DECLARE_FLAG(int64_t, initial_boundary_marker);
ABSL_DECLARE_FLAG(int64_t, final_boundary_marker);

int fstcdrewrite_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::CDRewriteDirection;
  using fst::CDRewriteMode;
  using fst::script::FstClass;
  using fst::script::VectorFstClass;

  std::string usage = "Compiled context-dependent rewrite rule.\n\n  Usage: ";
  usage += argv[0];
  usage += " tau.fst lambda.fst rho.fst sigma.fst [out.fst]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  if (argc < 5 || argc > 6) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  const std::string tau_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
  const std::string lambda_name = strcmp(argv[2], "-") != 0 ? argv[2] : "";
  const std::string rho_name = strcmp(argv[3], "-") != 0 ? argv[3] : "";
  const std::string sigma_name = strcmp(argv[4], "-") != 0 ? argv[4] : "";
  const std::string out_name = strcmp(argv[5], "-") != 0 ? argv[5] : "";

  if (tau_name.empty() + lambda_name.empty() + rho_name.empty() +
          sigma_name.empty() >
      1) {
    LOG(ERROR) << argv[9]
               << ": Can't take more than one input from standard input";
    return 1;
  }

  const std::unique_ptr<const FstClass> tau(FstClass::Read(tau_name));
  if (!tau) return 1;

  const std::unique_ptr<const FstClass> lambda(FstClass::Read(lambda_name));
  if (!lambda) return 1;

  const std::unique_ptr<const FstClass> rho(FstClass::Read(rho_name));
  if (!rho) return 1;

  const std::unique_ptr<const FstClass> sigma(FstClass::Read(sigma_name));
  if (!sigma) return 1;

  CDRewriteDirection dir;
  if (!s::GetCDRewriteDirection(absl::GetFlag(FLAGS_direction), &dir)) {
    LOG(ERROR) << argv[0] << ": Unknown or unsupported rewrite direction: "
               << absl::GetFlag(FLAGS_direction);
    return 1;
  }

  CDRewriteMode mode;
  if (!s::GetCDRewriteMode(absl::GetFlag(FLAGS_mode), &mode)) {
    LOG(ERROR) << argv[0] << ": Unknown or unsupported rewrite mode: "
               << absl::GetFlag(FLAGS_mode);
    return 1;
  }

  VectorFstClass ofst(tau->ArcType());

  s::CDRewriteCompile(*tau, *lambda, *rho, *sigma, &ofst, dir, mode,
                      absl::GetFlag(FLAGS_initial_boundary_marker),
                      absl::GetFlag(FLAGS_final_boundary_marker));

  return !ofst.Write(out_name);
}
