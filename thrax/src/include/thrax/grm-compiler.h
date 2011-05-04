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
// This is the main compiler class that takes in a source file and calls the
// parser to produce an AST and then walks that AST to generate the desired
// FSTs.  These FSTs are then loaded into a GrmManager.

#ifndef NLP_GRM_LANGUAGE_GRM_COMPILER_H_
#define NLP_GRM_LANGUAGE_GRM_COMPILER_H_

#include <string>
#include <vector>
using std::vector;

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/arc.h>
#include <thrax/grm-manager.h>
#include <thrax/lexer.h>

namespace thrax {

class Namespace;
class Node;

class GrmCompiler {
 public:
  typedef fst::StdArc Arc;

  GrmCompiler();
  ~GrmCompiler();

  // ***************************************************************************
  // COMPILATION: These functions load up data into the GrmCompiler.
  // Either:
  //   1.) call Parse*() followed by EvaluateAst(), or
  //   2.) load up an existing FST Archive by using LoadArchive().

  // Parses the provided grammar data via the filename or the file contents.
  // Defined in parser.y.  Returns true on success and false on failure.
  bool ParseFile(const string& filename);
  bool ParseContents(const string& contents);

  // Evaluate the AST from scratch, creating a new walker with no preset
  // environment.  Returns true on success and false on failure.
  bool EvaluateAst() { return EvaluateAstWithEnvironment(NULL); }

  // Evaluate the AST using the provided environment namespace.  This is likely
  // for imported files and modules and should really only be called by AST
  // walkers (i.e., not by users).  Call using NULL to create a new environment.
  // Returns true on success and false on failure.
  bool EvaluateAstWithEnvironment(Namespace* env);

  // ***************************************************************************
  // The following functions give access to, modify, or serialize internal data.

  Lexer* GetLexer() { return &lexer_; }

  void SetAst(Node* root);           // Adds a new AST for this compiler.
  Node* GetAst() { return root_; };  // Gets the most recently AST.

  // Returns a pointer to the grammar manager so that we can perform rewrites
  // (or exports, or whatever).  This pointer remains owned by this class,
  // however, so it should not be deleted by the caller.
  const GrmManager* GetGrmManager() const { return &grm_manager_; }

  // ***************************************************************************
  // Various other useful functions.

  // Sets the parsing to failure.  If provided with a non-empty message, then
  // we'll print that out for the user.  If the message is empty, print out
  // nothing (and just silently fail the parse/compile).
  void Error(const string& message);

 private:
  Lexer lexer_;

  vector<Node*> asts_;  // The list of actual ASTs owned by this compiler.
  Node* root_;          // A pointer to the most recent AST.

  GrmManager grm_manager_;  // The manager that holds all of the FSTs.

  bool success_;

  DISALLOW_COPY_AND_ASSIGN(GrmCompiler);
};

}  // namespace thrax

#endif  // NLP_GRM_LANGUAGE_GRM_COMPILER_H_
