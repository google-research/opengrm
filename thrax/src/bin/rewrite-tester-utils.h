// Copyright 2005-2020 Google LLC
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
//
#ifndef NLP_GRM_LANGUAGE_UTIL_REWRITE_TESTER_UTILS_H_
#define NLP_GRM_LANGUAGE_UTIL_REWRITE_TESTER_UTILS_H_

#include <memory>
#include <string>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/arc.h>
#include <fst/fst.h>
#include <fst/string.h>
#include <fst/symbol-table.h>
#include <thrax/grm-manager.h>
#include <../bin/utildefs.h>

class RewriteTesterUtils {
 public:
  RewriteTesterUtils();

  ~RewriteTesterUtils() = default;

  void Initialize();

  void Run();

  // Runs the input through the FSTs. Prepends "Output string:" to each line if
  // prepend_output is true
  const std::string ProcessInput(const std::string& input,
                                 bool prepend_output = true);

 private:
  // Reader for the input in interactive version.
  bool ReadInput(std::string* s);

  ::thrax::GrmManagerSpec<::fst::StdArc> grm_;
  std::vector<std::string> rules_;
  std::unique_ptr<::fst::StringCompiler<::fst::StdArc>> compiler_;
  std::unique_ptr<::fst::SymbolTable> byte_symtab_;
  std::unique_ptr<::fst::SymbolTable> utf8_symtab_;
  std::unique_ptr<::fst::SymbolTable> generated_symtab_;
  std::unique_ptr<::fst::SymbolTable> input_symtab_;
  ::fst::TokenType type_;
  std::unique_ptr<::fst::SymbolTable> output_symtab_;

  RewriteTesterUtils(const RewriteTesterUtils&) = delete;
  RewriteTesterUtils& operator=(const RewriteTesterUtils&) = delete;
};

#endif  // NLP_GRM_LANGUAGE_UTIL_REWRITE_TESTER_UTILS_H_
