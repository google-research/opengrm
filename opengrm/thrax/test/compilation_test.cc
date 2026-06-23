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

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "opengrm/compat/file.h"

#include "openfst/compat/file_path.h"
#include "gtest/gtest.h"
#include "absl/log/check.h"
#include "absl/strings/str_cat.h"
#include "openfst/extensions/far/far.h"
#include "openfst/lib/arc.h"
#include "opengrm/thrax/ast/node.h"
#include "opengrm/thrax/grm-compiler.h"
#include "opengrm/thrax/grm-manager.h"
#include "opengrm/thrax/test/total-sort.h"
#include "opengrm/thrax/walker/printer.h"
#include "opengrm/thrax/walker/walker.h"

namespace thrax {
namespace {

using ::fst::FarReader;
using ::fst::StdArc;

class CompilationTest : public ::testing::Test {
 protected:
  CompilationTest() {
    const std::string testdata_dir = fst::JoinPath(
        std::string("."),
        "opengrm/thrax/test/testdata/compilation");

    basic_grm_path_ = fst::JoinPath(testdata_dir, "basic.grm");
    basic_golden_path_ = fst::JoinPath(testdata_dir, "basic.sttable.far");
    basic_output_path_ = fst::JoinPath(::testing::TempDir(), "basic.far");

    stringboundaries_grm_path_ =
        fst::JoinPath(testdata_dir, "stringboundaries.grm");
    stringboundaries_golden_path_ =
        fst::JoinPath(testdata_dir, "stringboundaries.sttable.far");
    stringboundaries_output_path_ =
        fst::JoinPath(::testing::TempDir(), "stringboundaries.far");

    advanced_grm_path_ = fst::JoinPath(testdata_dir, "advanced.grm");
    advanced_golden_path_ =
        fst::JoinPath(testdata_dir, "advanced.sttable.far");
    advanced_output_path_ =
        fst::JoinPath(::testing::TempDir(), "advanced.far");

    import_grm_path_ = fst::JoinPath(testdata_dir, "import.grm");
    import_golden_path_ = fst::JoinPath(testdata_dir, "import.sttable.far");
    import_output_path_ = fst::JoinPath(::testing::TempDir(), "import.far");

    ast_grm_path_ = fst::JoinPath(testdata_dir, "ast.grm");
    ast_txt_path_ = fst::JoinPath(testdata_dir, "ast.txt");
  }

  void FarsEqual(const std::string& golden_file,
                 const std::string& output_file) {
    std::unique_ptr<FarReader<StdArc>> gfar(
        FarReader<StdArc>::Open(golden_file));
    std::unique_ptr<FarReader<StdArc>> ofar(
        FarReader<StdArc>::Open(output_file));

    for (; !gfar->Done() && !ofar->Done(); gfar->Next(), ofar->Next()) {
      ASSERT_EQ(gfar->GetKey(), ofar->GetKey());
      const auto* gfst = gfar->GetFst();
      const auto* ofst = ofar->GetFst();
      EXPECT_TRUE(EqualUnordered(*gfst, *ofst)) << gfar->GetKey();
    }

    ASSERT_TRUE(gfar->Done());
    ASSERT_TRUE(ofar->Done());
  }

  std::string basic_grm_path_;
  std::string basic_golden_path_;
  std::string basic_output_path_;
  std::string stringboundaries_grm_path_;
  std::string stringboundaries_golden_path_;
  std::string stringboundaries_output_path_;
  std::string advanced_grm_path_;
  std::string advanced_golden_path_;
  std::string advanced_output_path_;
  std::string import_grm_path_;
  std::string import_golden_path_;
  std::string import_output_path_;
  std::string ast_grm_path_;
  std::string ast_txt_path_;
};

TEST_F(CompilationTest, BasicTest) {
  GrmCompilerSpec<StdArc> grm;
  ASSERT_TRUE(grm.ParseFile(basic_grm_path_));
  ASSERT_TRUE(grm.EvaluateAst());
  grm.GetGrmManager()->ExportFar(basic_output_path_);

  FarsEqual(basic_golden_path_, basic_output_path_);

  // Check a few of the rewrites to make sure that we can immediately rewrite
  // using the newly compiled rules.
  std::string output;
  EXPECT_TRUE(grm.GetGrmManager()->RewriteBytes("b1", "b", &output));
  EXPECT_FALSE(grm.GetGrmManager()->RewriteBytes("b1", "cupcake", &output));
  EXPECT_TRUE(grm.GetGrmManager()->RewriteBytes("a_to_b", "a", &output));
  EXPECT_EQ("b", output);
}

TEST_F(CompilationTest, StringBoundaryTest) {
  GrmCompilerSpec<StdArc> grm;
  ASSERT_TRUE(grm.ParseFile(stringboundaries_grm_path_));
  ASSERT_TRUE(grm.EvaluateAst());
  grm.GetGrmManager()->ExportFar(stringboundaries_output_path_);

  FarsEqual(stringboundaries_golden_path_, stringboundaries_output_path_);

  // Check a few of the rewrites to make sure that we can immediately rewrite
  // using the newly compiled rules.
  std::string output;
  EXPECT_TRUE(grm.GetGrmManager()->RewriteBytes("beg", "abc", &output));
  EXPECT_EQ("bc", output);
  EXPECT_TRUE(grm.GetGrmManager()->RewriteBytes("beg", "bca", &output));
  EXPECT_EQ("bca", output);
  EXPECT_TRUE(grm.GetGrmManager()->RewriteBytes("end", "abc", &output));
  EXPECT_EQ("abc", output);
  EXPECT_TRUE(grm.GetGrmManager()->RewriteBytes("end", "bca", &output));
  EXPECT_EQ("bc", output);
  EXPECT_TRUE(grm.GetGrmManager()->RewriteBytes("ins_a", "", &output));
  EXPECT_EQ("a", output);
}

TEST_F(CompilationTest, AdvancedTest) {
  GrmCompilerSpec<StdArc> grm;
  ASSERT_TRUE(grm.ParseFile(advanced_grm_path_));
  ASSERT_TRUE(grm.EvaluateAst());
  grm.GetGrmManager()->ExportFar(advanced_output_path_);

  FarsEqual(advanced_golden_path_, advanced_output_path_);
}

TEST_F(CompilationTest, ImportTest) {
  GrmCompilerSpec<StdArc> grm;
  ASSERT_TRUE(grm.ParseFile(import_grm_path_));
  ASSERT_TRUE(grm.EvaluateAst());
  grm.GetGrmManager()->ExportFar(import_output_path_);

  FarsEqual(import_golden_path_, import_output_path_);
}

TEST_F(CompilationTest, AstTest) {
  GrmCompilerSpec<StdArc> grm;
  ASSERT_TRUE(grm.ParseFile(ast_grm_path_));
  ASSERT_TRUE(grm.EvaluateAst());

  std::ostringstream oss(std::ostringstream::out);
  AstPrinter printer(oss);
  Node* root = grm.GetAst();
  root->Accept(&printer);

  std::string golden_ast;
  CHECK_OK(file::ReadFileToString(ast_txt_path_, &golden_ast));

  // oss.str() should return a string already, but the test macro gets confused,
  // so we'll explicitly specify the type via a cast.
  EXPECT_EQ(golden_ast, static_cast<std::string>(oss.str()));
}

// Tests that each Identifier in an AST has proper beginning byte position.
class IdentifierPosTest : public ::testing::Test {
 private:
  // A simple walker that collects all IdentifierNode's into an outside vector.
  class IdentifierNodeCollector : public AstWalker {
   public:
    // IdentifierNodeCollector does not own the pointer.
    explicit IdentifierNodeCollector(std::vector<const IdentifierNode*>* nodes)
        : nodes_(nodes) {}

    // This type is neither copyable nor movable.
    IdentifierNodeCollector(const IdentifierNodeCollector&) = delete;
    IdentifierNodeCollector& operator=(const IdentifierNodeCollector&) = delete;

    void Visit(CollectionNode* node) override {
      for (int i = 0; i < node->Size(); ++i) {
        node->Get(i)->Accept(this);
      }
    }

    void Visit(FstNode* node) override {
      for (int i = 0; i < node->NumArguments(); ++i) {
        node->GetArgument(i)->Accept(this);
      }
    }

    void Visit(FunctionNode* node) override {
      node->GetName()->Accept(this);
      node->GetArguments()->Accept(this);
      node->GetBody()->Accept(this);
    }

    void Visit(GrammarNode* node) override {
      node->GetImports()->Accept(this);
      node->GetFunctions()->Accept(this);
      node->GetStatements()->Accept(this);
    }

    void Visit(IdentifierNode* node) override { nodes_->push_back(node); }

    void Visit(ImportNode* node) override {
      node->GetPath()->Accept(this);
      node->GetAlias()->Accept(this);
    }

    void Visit(RepetitionFstNode* node) override {
      for (int i = 0; i < node->NumArguments(); ++i) {
        node->GetArgument(i)->Accept(this);
      }
    }

    void Visit(ReturnNode* node) override { node->Get()->Accept(this); }

    void Visit(RuleNode* node) override {
      node->GetName()->Accept(this);
      node->Get()->Accept(this);
    }

    void Visit(StatementNode* node) override { node->Get()->Accept(this); }

    void Visit(StringFstNode* node) override {
      for (int i = 0; i < node->NumArguments(); ++i) {
        node->GetArgument(i)->Accept(this);
      }
    }

    void Visit(StringNode* node) override {}

   private:
    std::vector<const IdentifierNode*>* nodes_;
  };

 protected:
  void Run(const std::string& name) {
    std::string filename = fst::JoinPath(
        std::string("."),
        absl::StrCat(
            "opengrm/thrax/test/testdata/compilation/",
            name, ".grm"));
    std::string source;
    QCHECK_OK(file::ReadFileToString(filename, &source));

    GrmCompilerSpec<StdArc> grm;
    ASSERT_TRUE(grm.ParseFile(filename));

    std::vector<const IdentifierNode*> nodes;
    IdentifierNodeCollector walker(&nodes);
    grm.GetAst()->Accept(&walker);

    ASSERT_GT(nodes.size(), 0);

    for (auto i : nodes) {
      ASSERT_GE(i->GetBeginPos(), 0);
      ASSERT_LT(i->GetBeginPos(), source.size());
      std::string str = source.substr(i->GetBeginPos(), i->Get().size());
      EXPECT_EQ(i->Get(), str);
    }
  }
};

// Run the test on different input files.
TEST_F(IdentifierPosTest, Basic) { Run("basic"); }
TEST_F(IdentifierPosTest, Ast) { Run("ast"); }
TEST_F(IdentifierPosTest, Advanced) { Run("advanced"); }
TEST_F(IdentifierPosTest, Import) { Run("import"); }
TEST_F(IdentifierPosTest, StringBoundaries) { Run("stringboundaries"); }

}  // namespace
}  // namespace thrax
