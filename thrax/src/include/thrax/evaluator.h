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
#ifndef THRAX_EVALUATOR_H_
#define THRAX_EVALUATOR_H_

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <thrax/compat/utils.h>
#include <fst/extensions/far/far.h>
#include <fst/arc.h>
#include <fst/concat.h>
#include <fst/fst.h>
#include <fst/topsort.h>
#include <fst/vector-fst.h>
#include <fst/weight.h>
#include <thrax/collection-node.h>
#include <thrax/fst-node.h>
#include <thrax/function-node.h>
#include <thrax/grammar-node.h>
#include <thrax/identifier-node.h>
#include <thrax/import-node.h>
#include <thrax/node.h>
#include <thrax/return-node.h>
#include <thrax/rule-node.h>
#include <thrax/statement-node.h>
#include <thrax/string-node.h>
#include <thrax/grm-compiler.h>
#include <thrax/identifier-counter.h>
#include <thrax/printer.h>
#include <thrax/datatype.h>
#include <thrax/function.h>
#include <thrax/optimize.h>
#include <thrax/stringfst.h>
#include <thrax/symbols.h>
#include <thrax/namespace.h>
#include <thrax/walker.h>
#include <unordered_set>
#include <fst/compat.h>
#include <thrax/compat/stlfunctions.h>

DECLARE_bool(always_export);
DECLARE_bool(optimize_all_fsts);
DECLARE_bool(print_rules);
DECLARE_bool(save_symbols);
DECLARE_string(indir);

namespace thrax {

// The name of the special FST that holds the symbol table of the generated
// labels. This name should be one disallowed by the variable naming rules
// (i.e., the user shouldn't be able to create this).
static const char kStringFstSymtabFst[] = "*StringFstSymbolTable";

// Retrieves a C++ function for the proper arc type. See
// evaluator-specializations.cc for implementations.
template <typename Arc>
function::Function<Arc>* GetFunction(const std::string& func_name);

template <>
function::Function<::fst::StdArc>* GetFunction(
    const std::string& func_name);

template <>
function::Function<::fst::LogArc>* GetFunction(
    const std::string& func_name);

template <typename Arc>
class GrmCompilerSpec;

template <typename Arc>
class AstEvaluator : public AstWalker {
 public:
  using Transducer = ::fst::Fst<Arc>;
  using MutableTransducer = ::fst::VectorFst<Arc>;
  using LabelMapper = std::map<int64_t, int64_t>;

  // This constructor sets up the evaluator to run all nodes using a new
  // environment namespace.
  AstEvaluator()
      : AstWalker(),
        env_(new Namespace()),
        id_counter_(nullptr),
        run_all_(true),
        return_value_(nullptr),
        success_(true),
        optimize_embedding_(-1) {
    // This primary namespace should be the one that corresponds to the main
    // file being compiled.
    env_->SetTopLevel();
    // If we're parsing the entire file, then we need some space for local
    // variables (as we actually execute the body).
    env_->PushLocalEnvironment();
  }

  // This constructor will only run import and function nodes, loading them into
  // the provided namespace.
  explicit AstEvaluator(Namespace* env)
      : AstWalker(),
        env_(env),
        id_counter_(nullptr),
        run_all_(false),
        return_value_(nullptr),
        success_(true),
        optimize_embedding_(-1) {}

  ~AstEvaluator() override {
    // We only own the environment if we ran all of the nodes. Similarly, if we
    // run all of the nodes then we should've created one layer of local
    // variable space. Finally, the top-level grammar should own all of the
    // parsed ASTs via the grammars, so we'll kill those, too.
    if (run_all_) {
      env_->PopLocalEnvironment();
      delete env_;
      STLDeleteElements(&loaded_grammars_);
    }
  }

  // Three functions to control error handling in the case of bad input
  // grammars.

  // Call this before a new compilation to reset the state of the success_
  // variable.
  void ClearSuccess() { success_ = true; }

  bool Success() { return success_; }

  void Error(const Node& node, const std::string& message) {
    std::cout << file_ << ":" << node.getline() << ": " << message << std::endl;
    success_ = false;
  }

  void SetIdCounter(std::unique_ptr<AstIdentifierCounter> counter) {
    id_counter_ = std::move(counter);
  }

  void Visit(CollectionNode* node) override {
    LOG(FATAL) << "CollectionNode should not be visited; use the parent node.";
  }

  void Visit(FunctionNode* node) override {
    VLOG(2) << "Visiting Function";
    if (!Success()) return;
    const std::string& name = node->GetName()->Get();
    IdentifierNode function_inode(name);
    if (!env_->Get<FunctionNode>(function_inode)) {  // Add only if new.
      // The functions are held with the GrmCompilerSpec (more specifically the
      // abstract syntax node tree). So, we'll insert them without deletion now,
      // and kill them all at the end when we dispose of the grammar compilers.
      bool new_add = env_->InsertWithoutDelete(name, node);
      observed_function_names_.insert(name);
      if (!new_add) LOG(FATAL) << "Shadowing existing function: " << name;
    } else {
      if (observed_function_names_.find(name) !=
          observed_function_names_.end()) {
        LOG(WARNING) << "Duplicate function definition within file for "
                     << name << ". Ignoring.";
      }
    }
  }

  void Visit(GrammarNode* node) override {
    VLOG(2) << "Visiting GrammarNode";
    if (!Success()) return;
    CollectionNode* imports = node->GetImports();
    for (int i = 0; i < imports->Size(); ++i) (*imports)[i]->Accept(this);
    CollectionNode* functions = node->GetFunctions();
    for (int i = 0; i < functions->Size(); ++i) (*functions)[i]->Accept(this);
    if (run_all_) {
      CollectionNode* statements = node->GetStatements();
      for (int i = 0; i < statements->Size(); ++i) {
        StatementNode* stmt = fst::down_cast<StatementNode*>((*statements)[i]);
        if (stmt->GetType() == StatementNode::RETURN_STATEMENTNODE) {
          Error(*stmt, "Cannot return from main body");
          return;
        }
        stmt->Accept(this);
      }
    }
  }

  void Visit(IdentifierNode* node) override {
    LOG(FATAL) << "IdentifierNode should not be visited. Use Get() instead.";
  }

  void Visit(ImportNode* node) override {
    if (!Success()) return;
    // Gets (and checks) the path of the actual source and FAR.
    const auto& path =
        JoinPath(FST_FLAGS_indir, node->GetPath()->Get());
    if (Suffix(path) != "grm") {
      Error(*node, ::fst::StrCat("Extension for included files should be .grm: ",
                                path));
      return;
    }
    // Gets (and check) the alias name.
    const std::string& alias = node->GetAlias()->Get();
    if (alias.find('.') != std::string::npos) {
      Error(*node, ::fst::StrCat("Invalid import alias identifier: ", alias));
      return;
    }
    Namespace* prev_env = env_;
    env_ = env_->AddSubNamespace(path, alias);
    // Loads up the function source into the local environment.
    VLOG(2) << "Opening (and parsing) imported source file: " << path;
    if (!Readable(path)) {
      Error(*node, ::fst::StrCat("Unable to open grm source file: ", path));
      env_ = prev_env;
      return;
    }
    auto* grammar = new GrmCompilerSpec<Arc>();
    if (!grammar->ParseFile(path) ||
        !grammar->EvaluateAstWithEnvironment(env_, false)) {
      Error(*node,
            ::fst::StrCat("Errors while importing grm source file: ", path));
      env_ = prev_env;
      return;
    }
    loaded_grammars_.push_back(grammar);
    // Loads up the exported FSTs.
    std::string far_path = path.substr(0, path.length() - 3) + "far";
    VLOG(2) << "Opening (and loading FSTs from) companion far: " << far_path;
    auto far_reader =
        fst::WrapUnique(::fst::STTableFarReader<Arc>::Open(far_path));
    if (!far_reader) {
      Error(*node, ::fst::StrCat("Unable to open far archive: ", far_path));
      // We don't need to return or cleanup, as the next code will check for
      // Success() first, jumping to cleanup on failure.
    }
    // Looks for the special FST that holds the generated labels symbol table.
    // We must process that first so that we can know if any of the labels on
    // the incoming FSTs need to be reset.
    if (!far_reader->Done() && far_reader->Find(kStringFstSymtabFst)) {
      // Clears the remap, since any remappings that need to be done only safely
      // apply to the current FAR.
      function::StringFst<Arc>::ClearRemap();
      if (!function::StringFst<Arc>::MergeLabelSymbolTable(
              *far_reader->GetFst()->InputSymbols())) {
        Error(*node, "Failed to merge symbol tables");
        // We can gracefully exit this loop when success_ becomes false and do
        // the necessary cleanup afterwards.
      }
      far_reader->Reset();
    }
    // Adds the FSTs to that namespace. We'll loop through the reader, but quit
    // if we ever have an error.
    for (/* far_reader starts at the beginning */;
         Success() && !far_reader->Done(); far_reader->Next()) {
      const auto& key = far_reader->GetKey();
      if (key == kStringFstSymtabFst) {
        continue;
      } else {
        // Otherwise, we just have a normal exported FST. So we can just add it
        // into the variables.
        IdentifierNode key_inode(key);
        if (!env_->Get<DataType>(key_inode)) {  // Add only if new.
          // Must be mutable for now in case we need to change the symbol table.
          MutableTransducer tmpfst(*(far_reader->GetFst()));
          RemapGeneratedLabels(&tmpfst);
          ReassignSymbols(&tmpfst);
          auto fst = fst::WrapUnique(tmpfst.Copy());
          bool new_add =
              env_->Insert(key, std::make_unique<DataType>(std::move(fst)));
          if (!new_add) {
            LOG(FATAL) << "While loading " << path << " (aliased " << alias
                       << ") from file " << prev_env->GetFilename() << ", FST "
                       << key << " was clobbered.";
          }
        }
      }
    }

    // Restores the previous namespace.
    env_ = prev_env;
  }

  // Sets the return_value_ pointer to the FST described by the FstNode.
  void Visit(FstNode* node) override {
    VLOG(2) << "Visiting FstNode";
    if (!Success()) return;
    CHECK(return_value_ == nullptr);
    return_value_ = MakeFst(node);
  }

  void Visit(RepetitionFstNode* node) override {
    Visit(fst::implicit_cast<FstNode*>(node));
  }

  // This is pretty much a pass through that creates and binds the FST specified
  // in the ReturnNode.
  void Visit(ReturnNode* node) override {
    VLOG(2) << "Visiting ReturnNode";
    if (!Success()) return;
    node->Get()->Accept(this);
  }

  void Visit(RuleNode* node) override {
    VLOG(2) << "Visiting RuleNode";
    if (!Success()) return;
    IdentifierNode* identifier = node->GetName();
    if (FST_FLAGS_print_rules) {
      std::cout << "Evaluating rule: " << identifier->Get() << std::endl;
    }
    if (identifier->HasNamespaces()) {
      Error(*identifier,
            ::fst::StrCat("Cannot assign to an identifier within a namespace: ",
                         identifier->Get()));
      return;
    }
    const std::string& name = identifier->GetIdentifier();
    node->Get()->Accept(this);
    std::unique_ptr<DataType> thing = GetReturnValue();
    // Inserts the new variable, dying if it clobbers a pre-existing object.
    if (!env_->InsertLocal(name, std::move(thing))) {
      Error(*identifier,
            ::fst::StrCat("Cannot clobber existing variable: ", name));
      return;
    }
    if (node->ShouldExport()) {
      if (env_->LocalEnvironmentDepth() == 1) {
        exported_fsts_.insert(identifier);
      } else if (!FST_FLAGS_always_export) {
        Error(*identifier,
              ::fst::StrCat("Variables may only be exported from the top-level "
                           "grammar: ",
                           name));
        return;
      }
    }
  }

  // Visiting a StatementNode simply passes on the Accept() request to the
  // underlying node.
  void Visit(StatementNode* node) override {
    VLOG(2) << "Visiting StatementNode";
    if (!Success()) return;
    node->Get()->Accept(this);
  }

  void Visit(StringFstNode* node) override {
    Visit(fst::implicit_cast<FstNode*>(node));
  }

  // Sets the return_value_ with the value of the string.
  void Visit(StringNode* node) override {
    VLOG(2) << "Visiting StringNode";
    if (!Success()) return;
    CHECK(return_value_ == nullptr);
    return_value_ = std::make_unique<DataType>(node->Get());
  }

  // Inserts the exported FSTs from this file into the provided map.
  // Preexisting data with the same key will be clobbered.
  //
  // Boolean argument top_level indicates whether or not the Evaluator is
  // currently dealing with a top level grammar file (i.e. not an imported
  // grammar). This information gets passed down ultimately to StringFst's
  // GetLabelSymbolTable to determine (assuming --save_symbols is set), whether
  // or not to add generated labels to the byte and utf8 symbol tables.
  void GetFsts(std::map<std::string, std::unique_ptr<const Transducer>>* fsts,
               bool top_level) {
    // Checks if we ever used generated labels. If so, get the symbol table and
    // add it to a unique FST called kStringFstSymtabFst.
    if (auto generated_labels =
            function::StringFst<Arc>::GetLabelSymbolTable(top_level)) {
      auto label_fst = std::make_unique<MutableTransducer>();
      label_fst->SetInputSymbols(generated_labels.get());
      (*fsts)[kStringFstSymtabFst] = std::move(label_fst);
    }

    // Gets the exported FSTs and add them to the map.
    for (const auto* fst_i : exported_fsts_) {
      const std::string& name = fst_i->Get();
      // If always_export is set, an imported function, which in turn contains a
      // named fst variable, will have that variable exported when the function
      // is expanded in the top-level grammar, but that FST is not really part
      // of the top-level environment, so it fails to find it here and will die.
      // The choices are to try to make the compilation more clever about this
      // in the first place, which is hairy, or catch it here.
      if (env_->Get<DataType>(*fst_i) == nullptr) {
        LOG(WARNING) << "Cannot find exportable fst with name "
                     << name
                     << ": ignoring.";
        continue;
      }
      VLOG(1) << "Expanding FST: " << name;
      if (!env_->Get<DataType>(*fst_i)->template is<Transducer*>()) {
        Error(*fst_i,
              ::fst::StrCat("Cannot export non-FST variable: ", fst_i->Get()));
        return;
      }
      Transducer* fst =
          *env_->Get<DataType>(*fst_i)->template get<Transducer*>();
      auto nfst = std::make_unique<MutableTransducer>(*fst);
      // If the transducer has input symbols or output symbols, and if those are
      // either the byte symbol table or the utf8 symbol table, then we must
      // reassign those tables, since we may have added generated labels. In the
      // worst case this is a no-op.
      ReassignSymbols(nfst.get());
      // TopSort to address b/119868645.
      //
      // TODO(rws): The particular example in b/119868645 is evidently fixed by
      // this, but this is not guaranteed to work in general since TopSort is a
      // no-op on cyclic machines.
      TopSort(nfst.get());
      (*fsts)[name] = std::move(nfst);
    }
  }

  void set_file(const std::string& file) { file_ = file; }

 private:
  // Extracts and interprets the proper arguments from an FstNode and binds them
  // to the DataType union. On failure, we'll return nullptr.
  std::unique_ptr<std::vector<std::unique_ptr<DataType>>>
  GetArgumentsFromFstNode(FstNode* node, int num_arguments) {
    auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>();
    for (int i = 0; i < num_arguments; ++i) {
      node->GetArgument(i)->Accept(this);
      std::unique_ptr<DataType> return_value = GetReturnValue();
      if (!return_value) {
        return nullptr;
      }
      args->push_back(std::move(return_value));
    }
    return args;
  }

  // Does the same as above, but using a CollectionNode instead.
  std::unique_ptr<std::vector<std::unique_ptr<DataType>>>
  GetArgumentsFromCollectionNode(CollectionNode* node) {
    auto args = std::make_unique<std::vector<std::unique_ptr<DataType>>>();
    for (int i = 0; i < node->Size(); ++i) {
      node->Get(i)->Accept(this);
      std::unique_ptr<DataType> return_value = GetReturnValue();
      if (!return_value) {
        return nullptr;
      }
      args->push_back(std::move(return_value));
    }
    return args;
  }

  // Given a function node and an enclosing namespace, this will first bind the
  // arguments (taking ownership of the arguments) and then execute the function
  // AST in the new namespace, returning the output DataType at the end. The
  // provided arguments are owned by this function.
  std::unique_ptr<DataType> MakeFstFromLocalFunction(
      FunctionNode* func_node, const Node& debug_location_node,
      Namespace* func_namespace,
      std::unique_ptr<std::vector<std::unique_ptr<DataType>>> arguments) {
    Namespace* prev_env = env_;
    env_ = func_namespace;
    env_->PushLocalEnvironment();
    // Creates a new layer of environment symbol table and binds the passed
    // arguments.
    CollectionNode* fa_node = func_node->GetArguments();
    if (fa_node->Size() != arguments->size()) {
      Error(debug_location_node,
            ::fst::StrCat("Expected ", fa_node->Size(), " arguments but got ",
                         arguments->size()));
    }
    for (int i = 0; Success() && i < fa_node->Size(); ++i) {
      IdentifierNode* fa_identifier =
          fst::down_cast<IdentifierNode*>((*fa_node)[i]);
      if (fa_identifier->HasNamespaces()) {
        Error(*fa_identifier, ::fst::StrCat("Invalid function argument: ",
                                           fa_identifier->Get()));
        break;
      }
      const std::string& arg_name = fa_identifier->GetIdentifier();
      auto arg = (*arguments)[i]->Copy();
      env_->InsertLocal(arg_name, std::move(arg));
    }
    // Iterates over the statements and runs each.
    CollectionNode* fb_node = func_node->GetBody();
    std::unique_ptr<DataType> output = nullptr;
    for (int i = 0; Success() && i < fb_node->Size(); ++i) {
      StatementNode* stmt = fst::down_cast<StatementNode*>((*fb_node)[i]);
      stmt->Accept(this);
      if (stmt->GetType() == StatementNode::RETURN_STATEMENTNODE) {
        output = GetReturnValue();
        break;
      }
    }
    // Tosses out the function-scope environment.
    env_->PopLocalEnvironment();
    env_ = prev_env;
    return output;
  }

  // Looks up a C++ function by name from the registerer and then executes that
  // function on that provided arguments. This function takes ownership of the
  // arguments (via the C++ functions). This returns the new object saved into
  // the output argument. This function returns true if the function was found
  // and false otherwise.
  std::unique_ptr<DataType> MakeFstFromCFunction(
      const std::string& function_name, const Node& debug_location_node,
      std::unique_ptr<std::vector<std::unique_ptr<DataType>>> arguments) {
    function::Function<Arc>* func = GetFunction<Arc>(function_name);
    // If we get a nullptr function, then the name was invalid.
    if (!func) return nullptr;
    auto output = func->Run(std::move(arguments));
    if (!output) Error(debug_location_node, "C++ function call failed");
    return output;
  }

  // The main evaluator function---this takes in an FstNode and returns the
  // object specified. Note that this is a misnomer (for historical
  // reasons)---we can actually return things other than an FST (whatever
  // DataType supports).
  std::unique_ptr<DataType> MakeFst(FstNode* node) {
    std::unique_ptr<DataType> output;
    switch (node->GetType()) {
      case FstNode::CONCAT_FSTNODE: {
        VLOG(2) << "Concat Fst:";
        auto args = GetArgumentsFromFstNode(node, 2);
        output = MakeFstFromCFunction("Concat", *node, std::move(args));
        CHECK_NE(output, nullptr);
        break;
      }
      case FstNode::UNION_FSTNODE: {
        VLOG(2) << "Union Fst:";
        auto args = GetArgumentsFromFstNode(node, 2);
        output = MakeFstFromCFunction("Union", *node, std::move(args));
        CHECK_NE(output, nullptr);
        break;
      }
      case FstNode::DIFFERENCE_FSTNODE: {
        VLOG(2) << "Difference Fst:";
        auto args = GetArgumentsFromFstNode(node, 2);
        output = MakeFstFromCFunction("Difference", *node, std::move(args));
        CHECK_NE(output, nullptr);
        break;
      }
      case FstNode::REWRITE_FSTNODE: {
        VLOG(2) << "Rewrite Fst:";
        auto args = GetArgumentsFromFstNode(node, 2);
        output = MakeFstFromCFunction("Rewrite", *node, std::move(args));
        CHECK_NE(output, nullptr);
        break;
      }
      case FstNode::REPETITION_FSTNODE: {
        VLOG(2) << "Repetition Fst:";
        auto args = GetArgumentsFromFstNode(node, 1);
        RepetitionFstNode* rnode = fst::down_cast<RepetitionFstNode*>(node);
        args->push_back(std::make_unique<DataType>(
            static_cast<int>(rnode->GetRepetitionType())));
        if (rnode->GetRepetitionType() == RepetitionFstNode::RANGE) {
          int min;
          int max;
          rnode->GetRange(&min, &max);
          args->push_back(std::make_unique<DataType>(min));
          args->push_back(std::make_unique<DataType>(max));
        }
        output = MakeFstFromCFunction("Closure", *node, std::move(args));
        CHECK_NE(output, nullptr);
        break;
      }
      case FstNode::COMPOSITION_FSTNODE: {
        VLOG(2) << "Composition Fst:";
        if (optimize_embedding_ > -1) optimize_embedding_++;
        if (optimize_embedding_ > 1) node->SetOptimize();
        auto args = GetArgumentsFromFstNode(node, 2);
        args->push_back(
            std::make_unique<DataType>("right"));  // ArcSort the right FST.
        output = MakeFstFromCFunction("Compose", *node, std::move(args));
        CHECK_NE(output, nullptr);
        break;
      }
      case FstNode::IDENTIFIER_FSTNODE: {
        IdentifierNode* identifier =
            fst::down_cast<IdentifierNode*>(node->GetArgument(0));
        VLOG(2) << "Identifier Fst: " << identifier->Get();
        DataType* original = env_->Get<DataType>(*identifier);
        if (!original) {
          Error(*identifier,
                ::fst::StrCat("Undefined symbol: ", identifier->Get()));
          return nullptr;
        }
        output = original->Copy();
        // If we're currently at the top level namespace (i.e., compiling the
        // main body of the primary compilation target), and if the identifier
        // is one without aliases, then we may wish to free the memory if this
        // is the final access.
        if (env_->IsTopLevel() &&
            env_->LocalEnvironmentDepth() == 1 &&
            !identifier->HasNamespaces() &&
            !id_counter_->Decrement(identifier->GetIdentifier())) {
          VLOG(3) << "Erasing local variable: " << identifier->GetIdentifier();
          CHECK(env_->EraseLocal(identifier->GetIdentifier()));
        }
        break;
      }
      case FstNode::STRING_FSTNODE: {
        StringFstNode* snode = fst::down_cast<StringFstNode*>(node);
        const std::string& text =
            fst::down_cast<StringNode*>(node->GetArgument(0))->Get();
        VLOG(2) << "String Fst: " << text;
        auto args =
            std::make_unique<std::vector<std::unique_ptr<DataType>>>(2);
        (*args)[0] = std::make_unique<DataType>(
            static_cast<int>(snode->GetParseMode()));
        (*args)[1] = std::make_unique<DataType>(text);
        if (snode->GetParseMode() == StringFstNode::SYMBOL_TABLE) {
          // In the case of the symbol table, we need to evaluate its node and
          // pick up the actual table. Error checking will be handled by the
          // StringFst function.
          snode->GetArgument(1)->Accept(this);
          std::unique_ptr<DataType> thing = GetReturnValue();
          args->push_back(std::move(thing));
        }
        output = MakeFstFromCFunction("StringFst", *node, std::move(args));
        CHECK_NE(output, nullptr);
        break;
      }
      case FstNode::FUNCTION_FSTNODE: {
        IdentifierNode* func_identifier_node =
            fst::down_cast<IdentifierNode*>(node->GetArgument(0));
        const std::string& name = func_identifier_node->Get();
        VLOG(2) << "Function Call Fst: " << name;
        if (name == "Optimize") optimize_embedding_ = 0;
        auto args = GetArgumentsFromCollectionNode(
            fst::down_cast<CollectionNode*>(node->GetArgument(1)));
        if (!Success() || !args) {
          Error(*func_identifier_node,
                ::fst::StrCat("Unable to bind all arguments for function call: ",
                             func_identifier_node->Get()));
          return nullptr;
        }
        bool function_found = false;
        Namespace* func_namespace;
        FunctionNode* func_node =
            env_->Get<FunctionNode>(*func_identifier_node, &func_namespace);
        output = nullptr;
        if (func_node) {
          // Gets the function and make sure the names match and then calls the
          // function.
          CHECK_EQ(func_node->GetName()->Get(),
                    func_identifier_node->GetIdentifier());
          output = MakeFstFromLocalFunction(func_node, *func_identifier_node,
                                            func_namespace, std::move(args));
          function_found = true;
        } else if (!func_identifier_node->HasNamespaces()) {
          // If we didn't find a natural function with that name, then we might
          // have a C++ function, provided that the identifier has no
          // namespaces.
          output = MakeFstFromCFunction(func_identifier_node->GetIdentifier(),
                                        *func_identifier_node, std::move(args));
          if (output) function_found = true;
        }
        if (!function_found) {
          Error(*func_identifier_node,
                ::fst::StrCat("Undefined function identifier: ",
                             func_identifier_node->Get()));
          return nullptr;
        }
        // Now that we are done, set the optimize_embedding_ setting back to the
        // default.
        optimize_embedding_ = -1;
        break;
      }
      default: {
        LOG(FATAL) << "Unknown FstNode type: " << node->GetType();
      }
    }
    // If we have an FST as the output, then we might have some extra
    // post-processing.
    if (output && output->is<Transducer*>()) {
      // First, we can attach the weight if appropriate.
      if (node->HasWeight()) {
        Transducer* unweighted_fst = *output->get<Transducer*>();
        auto weighted_fst = AttachWeight(*unweighted_fst, node->GetWeight());
        output = std::make_unique<DataType>(std::move(weighted_fst));
      }
      // Now, we might wish to always optimize the FSTs, or the node is slated
      // to be optimized (e.g. a composition node within an Optimize
      // FUNCTION_FSTNODE).
      if (FST_FLAGS_optimize_all_fsts || node->ShouldOptimize()) {
        auto optimized_fst = function::Optimize<Arc>::ActuallyOptimize(
            **output->get<Transducer*>());
        output = std::make_unique<DataType>(std::move(optimized_fst));
        // This is the interesting case to be able to keep track of.
        if (node->ShouldOptimize())
          VLOG(2) << "Optimizing at line " << node->getline();
      }
    }
    return output;
  }

  // Releasess control of the return_value_ and returns it.
  std::unique_ptr<DataType> GetReturnValue() {
    return std::move(return_value_);
  }

  // Given an FST, we'll create a new one with the weight tacked on as a final
  // state.
  static std::unique_ptr<Transducer> AttachWeight(
      const Transducer& input_fst, const std::string& weight_str) {
    // Generates the appropriate weight.
    auto weight = Arc::Weight::One();
    std::istringstream iss(weight_str);
    iss >> weight;  // Interprets the weight instead of reading it as raw bytes.
    // Then, creates a single state FST with the appropriate final state weight.
    MutableTransducer weight_fst;
    int state = weight_fst.AddState();
    weight_fst.SetStart(state);
    weight_fst.SetFinal(state, weight);
    // If we are saving symbols then we have to add the symbol tables of our
    // input FST to this new single state FST.
    if (FST_FLAGS_save_symbols) {
      weight_fst.SetInputSymbols(input_fst.InputSymbols());
      weight_fst.SetOutputSymbols(input_fst.OutputSymbols());
    }
    // Glues the actual input FST to the front; this will automatically set the
    // weight properly.
    return std::make_unique<::fst::ConcatFst<Arc>>(input_fst, weight_fst);
  }

  // If FST_FLAGS_save_symbols is set for the system the imported FSTs will have
  // symbol tables. If these are either the byte symbol table or the utf8 symbol
  // table, they may have additional generated labels associated with them.
  // These generated labels will cause the symbol tables to potentially be in
  // conflict with other symbol tables either from other imported FSTs, or from
  // the ones created in this grammar. Since all of the generated labels from
  // all the imported files as well as the ones generated in this grammar are
  // stored in the StringFst class, and since these will be added back in to any
  // byte or utf8 symbol tables on write-out, the easiest thing to do is reset
  // the symbol tables to be just the basic byte or utf8 symbol tables, and then
  // when the new FSTs are written out, reconstruct all of the generated labels
  // in those symbol tables.
  void ReassignSymbols(MutableTransducer* fst) {
    if (fst->InputSymbols()) {
      if (fst->InputSymbols()->Name() ==
          function::kByteSymbolTableName) {
        fst->SetInputSymbols(function::GetByteSymbolTable());
      } else if (fst->InputSymbols()->Name() ==
                 function::kUtf8SymbolTableName) {
        fst->SetInputSymbols(function::GetUtf8SymbolTable());
      }
    }
    if (fst->OutputSymbols()) {
      if (fst->OutputSymbols()->Name() ==
          function::kByteSymbolTableName) {
        fst->SetOutputSymbols(function::GetByteSymbolTable());
      } else if (fst->OutputSymbols()->Name() ==
                 function::kUtf8SymbolTableName) {
        fst->SetOutputSymbols(function::GetUtf8SymbolTable());
      }
    }
  }

  // Remaps the generated labels of this FST using a StringFst's remap.
  void RemapGeneratedLabels(MutableTransducer* fst) {
    for (::fst::StateIterator<MutableTransducer> siter(*fst); !siter.Done();
         siter.Next()) {
      for (::fst::MutableArcIterator<MutableTransducer> aiter(
               fst, siter.Value());
           !aiter.Done(); aiter.Next()) {
        auto arc = aiter.Value();
        int64_t label = function::StringFst<Arc>::FindRemapLabel(arc.ilabel);
        if (label != ::fst::kNoLabel) arc.ilabel = label;
        label = function::StringFst<Arc>::FindRemapLabel(arc.olabel);
        if (label != ::fst::kNoLabel) arc.olabel = label;
        aiter.SetValue(arc);
      }
    }
  }

  Namespace* env_;  // Only owned if `run_all_` is true.
  std::unique_ptr<AstIdentifierCounter> id_counter_;
  const bool run_all_;

  // A list of the names of the FSTs we want exported at the end. We'll find
  // these FSTs from the local environment. Note that these pointers are owned
  // by the original AST, not us.
  std::set<IdentifierNode*> exported_fsts_;
  // A list of grammars that we've opened (and thus need to close at the end).
  // TODO(ttai): This is dangerous right now since if we have two simulatneous
  // compilations within the same process, then they'll be incorrectly sharing
  // this. It'd be nice to have a copy-function on the AST nodes so we can just
  // make a copy of the relevant bits of the AST (the function nodes) and store
  // just that.
  static std::vector<GrmCompilerSpec<Arc>*> loaded_grammars_;
  // This is the "return" datatype that is returned by a number of nodes.
  std::unique_ptr<DataType> return_value_;
  AstPrinter printer_;
  // Whether the compilation (evaluation) was a success.
  bool success_;
  // Name of file that we're currently evaluating.
  std::string file_;
  // Level of embedding under Optimize[] of a composition. If it is more than 1,
  // then optimize the composition. (If it is 1, then it's at the top level
  // under the Optimize[] and it will be optimized anyway.
  int optimize_embedding_;
  // Used in the evaluator to keep track of whether the same function name has
  // been defined in the current file more than once.
  std::set<std::string> observed_function_names_;

  AstEvaluator<Arc>(const AstEvaluator<Arc>&) = delete;
  AstEvaluator<Arc>& operator=(const AstEvaluator<Arc>&) = delete;
};

template <typename Arc>
std::vector<GrmCompilerSpec<Arc>*> AstEvaluator<Arc>::loaded_grammars_;

}  // namespace thrax

#endif  // THRAX_EVALUATOR_H_
