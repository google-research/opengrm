#include <thrax/algo/stripcomment.h>

#include <thrax/compat/utils.h>


namespace fst {
namespace {

std::string StripComment(const std::string &line) {
  char prev_char = '\0';
  for (size_t i = 0; i < line.size(); ++i) {
    const char this_char = line[i];
    if (this_char == '#' && prev_char != '\\') {
      // Strips comment and any trailing whitespace.
      return std::string(thrax::StripTrailingAsciiWhitespace(line.substr(0, i)));
    }
    prev_char = this_char;
  }
  return line;
}

std::string RemoveEscape(const std::string &line) {
  return thrax::StringReplace(StripComment(line), "\\#", "#", true);
}

}  // namespace

std::string StripCommentAndRemoveEscape(const std::string &line) {
  return RemoveEscape(StripComment(line));
}

}  // namespace fst
