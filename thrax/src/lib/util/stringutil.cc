#include <thrax/algo/stringutil.h>

#include <thrax/compat/utils.h>


namespace fst {
namespace {

std::string StripComment(const std::string &line) {
  char prev_char = '\0';
  for (size_t i = 0; i < line.size(); ++i) {
    const char this_char = line[i];
    if (this_char == '#' && prev_char != '\\') {
      // Strips comment and any trailing whitespace.
      return std::string(::fst::StripTrailingAsciiWhitespace(line.substr(0, i)));
    }
    prev_char = this_char;
  }
  return line;
}

std::string RemoveEscape(const std::string &line) {
  return thrax::StringReplace(line, "\\#", "#", true);
}

}  // namespace

std::string StripCommentAndRemoveEscape(const std::string &line) {
  return RemoveEscape(StripComment(line));
}

std::string Escape(const std::string &str) {
  std::string result;
  result.reserve(str.size());
  for (char ch : str) {
    switch (ch) {
      case '[':
      case ']':
      case '\\':
        result.push_back('\\');
    }
    result.push_back(ch);
  }
  return result;
}

}  // namespace fst
