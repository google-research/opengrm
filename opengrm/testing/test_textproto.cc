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

// Tests that rules read from a FAR matches textproto-based testdata.
//
// More specifically, it tests that:
//
// * The textproto can be parsed
// * For each input in the testproto, there are zero or more expected outputs,
//   or the failure bit is set
//
// While this is mostly used via `grm_textproto_test`, the following shows
// sample standalone usage:
//
//   cc_test(
//       name = "en_us_text_proto_test"
//       args = [
//           "--textproto_path=full/path/to/test/file",
//           "--far_path=full/path/to/far",
//       ],
//       linkstatic = 1,
//       data = [
//           ":path/to/test/file/relative/to/package,
//           ":far_label/relative/to/package,
//       ],
//       deps = [
//           "//testing/base/public:gunit_no_heapcheck",
//           "//third_party/opengrm/testing:test_textproto_lib",
//       ],
//   )

#include <iterator>
#include <string>
#include <vector>

#include "opengrm/compat/file.h"

#include "openfst/compat/file_path.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/algorithm/container.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "openfst/lib/string.h"
#include "openfst/script/getters.h"
#include "opengrm/rewrite/rule_cascade.h"
#include "opengrm/testing/testdata.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/text_format.h"

ABSL_FLAG(std::string, far_path, "", "Path of FAR to read rules from");
ABSL_FLAG(std::string, textproto_path, "", "Rewrites textproto path");
ABSL_FLAG(std::string, token_type, "byte",
          "Token type (one of \"byte\", \"utf8\")");
// See below to see how these modes are interpreted.
ABSL_FLAG(std::string, mode, "exact",
          "Rewrite mode (one of \"exact\", \"one_top\", \"subset\", \"top\")");

namespace opengrm {

enum class RewriteTestMode {
  kExact,
  kOneTop,
  kSubset,
  kTop,
};

namespace {

using ::fst::script::GetTokenType;
using ::rewrite::StdRuleCascade;
using ::testing::IsSupersetOf;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAreArray;

// Helper for computing set difference; C1 and C2 are containers (e.g., STL
// vectors, or protocol buffer repeated pointer fields) of strings.
template <class C1, class C2>
std::string StringSetDifference(const C1 &output1, const C2 &output2,
                                absl::string_view delimiter = ", ") {
  std::vector<std::string> difference;
  absl::c_set_difference(output1, output2, std::back_inserter(difference));
  return difference.empty() ? "" : absl::StrJoin(difference, delimiter);
}

class RewriteTextprotoTest : public testing::Test {
 public:
  RewriteTextprotoTest(StdRuleCascade *cascade, RewriteTestMode mode,
                       const Rewrite &rewrite)
      : cascade_(*cascade), mode_(mode), rewrite_(rewrite) {}

  void TestBody() override {
    ASSERT_TRUE(cascade_.SetRules(rewrite_.rule()));
    const std::string rules = absl::StrJoin(rewrite_.rule(), ", ");
    absl::string_view input = rewrite_.input();
    const google::protobuf::RepeatedPtrField<std::string> &expected = rewrite_.output();
    if (expected.empty()) {
      std::string actual;
      std::string debug;
      EXPECT_FALSE(cascade_.TopRewrite(input, &actual, &debug))
          << "Rule(s): " << rules << "\n"
          << "Expected rewrite failure for input: " << input << "\n"
          << "Actual output: " << actual << "\n"
          << "Debug output: " << debug << "\n";
    } else {
      switch (mode_) {
        case RewriteTestMode::kExact: {
          std::vector<std::string> actual;
          std::vector<std::string> debug;
          ASSERT_TRUE(cascade_.Rewrites(input, &actual, &debug))
              << "Rule(s): " << rules << "\n"
              << "Unexpected rewrites failure for input: " << input << "\n"
              << "Expected output: " << absl::StrJoin(expected, ", ") << "\n"
              << "Actual output: " << absl::StrJoin(actual, ", ") << "\n"
              << "Debug output: " << absl::StrJoin(debug, ", ");
          EXPECT_THAT(actual, UnorderedElementsAreArray(expected))
              << "Rule(s): " << rules << "\n"
              << "Failed for input: " << input << "\n"
              << "Expected output: " << absl::StrJoin(expected, ", ") << "\n"
              << "Actual output: " << absl::StrJoin(actual, ", ") << "\n"
              << "Debug output: " << absl::StrJoin(debug, ", ") << "\n"
              << "Extra elements: " << StringSetDifference(actual, expected)
              << "\n"
              << "Missing elements: " << StringSetDifference(expected, actual);
          break;
        }
        case RewriteTestMode::kOneTop: {
          std::string actual;
          std::string debug;
          ASSERT_THAT(expected, SizeIs(1))
              << "Rule(s): " << rules << "\n"
              << "Expected one top rewrite for input: " << input << "\n"
              << "but more than one output is listed: "
              << absl::StrJoin(expected, ", ");
          ASSERT_TRUE(cascade_.OneTopRewrite(input, &actual, &debug))
              << "Rule(s): " << rules << "\n"
              << "Expected one top rewrite for input: " << input << "\n"
              << "Actual output: " << actual << "\n"
              << "Debug output: " << debug;
          EXPECT_EQ(actual, expected[0])
              << "Rule(s): " << rules << "\n"
              << "Unexpected one top rewrite result input: " << input << "\n"
              << "Expected output: " << expected[0] << "\n"
              << "Actual output: " << actual << "\n"
              << "Debug output: " << debug;
          break;
        }
        case RewriteTestMode::kSubset: {
          std::vector<std::string> actual;
          std::vector<std::string> debug;
          ASSERT_TRUE(cascade_.Rewrites(input, &actual, &debug))
              << "Rule(s): " << rules << "\n"
              << "Unexpected rewrites failure for input: " << input << "\n"
              << "Expected output: " << absl::StrJoin(expected, ", ") << "\n"
              << "Actual output: " << absl::StrJoin(actual, ", ") << "\n"
              << "Debug output: " << absl::StrJoin(debug, ", ");
          EXPECT_THAT(actual, IsSupersetOf(expected))
              << "Rule(s): " << rules << "\n"
              << "Failed for input: " << input << "\n"
              << "Expected output: " << absl::StrJoin(expected, ", ") << "\n"
              << "Actual output: " << absl::StrJoin(actual, ", ") << "\n"
              << "Debug output: " << absl::StrJoin(debug, ", ") << "\n"
              << "Extra elements: " << StringSetDifference(actual, expected);
          break;
        }
        case RewriteTestMode::kTop: {
          std::string actual;
          std::string debug;
          ASSERT_THAT(expected, SizeIs(1))
              << "Rule(s): " << rules << "\n"
              << "Expected top rewrite for input: " << input << "\n"
              << "but more than one output is listed: "
              << absl::StrJoin(expected, ", ");
          ASSERT_TRUE(cascade_.TopRewrite(input, &actual, &debug))
              << "Rule(s): " << rules << "\n"
              << "Unexpected top rewrite failure for input: " << input << "\n"
              << "Expected output: " << expected[0] << "\n"
              << "Actual output: " << actual << "\n"
              << "Debug output: " << debug;
          EXPECT_EQ(actual, expected[0])
              << "Rule(s): " << rules << "\n"
              << "Unexpected top rewrite result input: " << input << "\n"
              << "Expected output: " << expected[0] << "\n"
              << "Actual output: " << actual << "\n"
              << "Debug output: " << debug;
          break;
        }
      }
    }
  }

 protected:
  StdRuleCascade &cascade_;
  const RewriteTestMode mode_;
  const Rewrite rewrite_;
};

}  // namespace

// Registers each rewrite test case as a separate test.
// Documentation: https://google.github.io/googletest/advanced.html#registering-tests-programmatically.
//
// This allows the tests to run independently of each other and can therefore
// be sharded, have accurate source location for each test case, and makes it
// possible to terminate a test case with ASSERT upon rewrite error and not
// proceed with subsequent EXPECT calls.
void RegisterTests(StdRuleCascade *cascade, RewriteTestMode mode,
                   absl::string_view path, const Rewrites &rewrites,
                   const google::protobuf::TextFormat::ParseInfoTree &parse_info) {
  // Take a copy of `far_path` to avoid the string going out of scope in the
  // lambda below.
  for (int i = 0; i < rewrites.rewrite_size(); ++i) {
    const Rewrite &rewrite = rewrites.rewrite(i);
    const google::protobuf::FieldDescriptor *rewrite_descriptor =
        Rewrites::GetDescriptor()->FindFieldByName("rewrite");
    google::protobuf::TextFormat::ParseLocation location =
        parse_info.GetLocation(rewrite_descriptor, i);
    testing::RegisterTest(
        /*test_suite_name=*/std::string(fst::Stem(path)).c_str(),
        /*test_name=*/absl::StrCat("Rewrite ", i).c_str(),
        /*type_param=*/nullptr, /*value_param=*/nullptr,
        /*file=*/std::string(path).c_str(), /*line=*/location.line,
        /*factory=*/[=]() {
          return new RewriteTextprotoTest(cascade, mode, rewrite);
        });
  }
}

absl::StatusOr<RewriteTestMode> GetMode(absl::string_view mode) {
  if (mode == "exact") return RewriteTestMode::kExact;
  if (mode == "one_top") return RewriteTestMode::kOneTop;
  if (mode == "subset") return RewriteTestMode::kSubset;
  if (mode == "top") return RewriteTestMode::kTop;
  return absl::InvalidArgumentError(absl::StrCat("Unrecognized mode: ", mode));
}

}  // namespace opengrm

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  absl::ParseCommandLine(argc, argv);

  absl::StatusOr<opengrm::RewriteTestMode> mode =
      opengrm::GetMode(absl::GetFlag(FLAGS_mode));
  QCHECK_OK(mode);  // Crash OK

  fst::TokenType token_type;
  QCHECK(opengrm::GetTokenType(absl::GetFlag(FLAGS_token_type),  // Crash OK
                               &token_type));

  rewrite::StdRuleCascade cascade(token_type);
  QCHECK_OK(cascade.LoadWithStatus(absl::GetFlag(FLAGS_far_path)));  // Crash OK

  absl::StatusOr<std::string> textproto =
      file::ReadFileToString(absl::GetFlag(FLAGS_textproto_path));
  QCHECK_OK(textproto);  // Crash OK

  google::protobuf::TextFormat::ParseInfoTree parse_info;
  google::protobuf::TextFormat::Parser parser;
  parser.WriteLocationsTo(&parse_info);
  opengrm::Rewrites rewrites;
  QCHECK(parser.ParseFromString(*textproto, &rewrites));  // Crash OK

  // Share the same cascade across all tests. Note that the tests are run
  // sequentially so mutating it with SetRules is fine.
  opengrm::RegisterTests(&cascade, *mode, absl::GetFlag(FLAGS_textproto_path),
                         rewrites, parse_info);
  return RUN_ALL_TESTS();
}
