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

#include <thrax/grm-compiler.h>

#include <map>
#include <string>
#include <vector>
using std::vector;

#include <thrax/node.h>
#include <thrax/evaluator.h>
#include <thrax/identifier-counter.h>
#include <thrax/printer.h>
#include <thrax/namespace.h>
#include <thrax/compat/stlfunctions.h>

DEFINE_bool(print_ast, false, "Whether we print out the AST to STDOUT.");

namespace thrax {

GrmCompiler::GrmCompiler()
    : root_(NULL) {}

GrmCompiler::~GrmCompiler() {
  STLDeleteContainerPointers(asts_.begin(), asts_.end());
}

void GrmCompiler::SetAst(Node* root) {
  asts_.push_back(root);
  root_ = root;
}

bool GrmCompiler::EvaluateAstWithEnvironment(Namespace* env) {
  if (!success_ || !root_) {
    cout << "Compile errors found." << endl;
    return false;
  }
  if (FLAGS_print_ast) {
    AstPrinter printer;
    root_->Accept(&printer);
  }

  VLOG(1) << "Commencing main compilation (AST evaluation).";
  AstEvaluator<Arc>* evaluator;
  if (env) {
    // If we have an environment, then we pass it to the Evaluator so that it
    // knows that we only want the includes.
    evaluator = new AstEvaluator<Arc>(env);
  } else {
    // We want to get a count of the identifiers so that we can free their
    // memory when the time comes.
    AstIdentifierCounter* id_counter = new AstIdentifierCounter();
    root_->Accept(id_counter);

    // If we don't have an environment, then we're doing the top level version,
    // where we execute the body.
    evaluator = new AstEvaluator<Arc>();
    evaluator->SetIdCounter(id_counter);
  }
  root_->Accept(evaluator);

  if (evaluator->Success()) {
    // We can always retrieve the FSTs.  If there are none (ex., since we're
    // only importing the file), this operation is still safe/fast.
    VLOG(1) << "Compilation complete.  Expanding exported FSTs.";
    evaluator->GetFsts(grm_manager_.GetFstMap());
  } else {
    cout << "Compilation failed." << endl;
    success_ = false;
  }

  delete evaluator;
  return success_;
}

void GrmCompiler::Error(const string& message) {
  success_ = false;
  if (!message.empty()) {
    cout << "****************************************\n"
         << "Line " << GetLexer()->line_number() << ": " << message << "\n"
         << "Context: "
         << GetLexer()->GetCurrentContext() << endl;
  }
}

}  // namespace thrax
