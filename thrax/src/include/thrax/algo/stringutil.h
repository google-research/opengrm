#ifndef FST_UTIL_STRING_STRINGUTIL_H_
#define FST_UTIL_STRING_STRINGUTIL_H_

#include <string>

namespace fst {

// Defines comment syntax for string files.
//
// The comment character is '#', and has scope until the end of the line. Any
// preceding whitespace before a comment is ignored.
//
// To use the '#' literal (i.e., to ensure it is not interpreted as the start of
// a comment) escape it with '\'; the escaping '\' in "\#" also removed.
std::string StripCommentAndRemoveEscape(const std::string &line);

// Escapes characters (namely, backslash and square brackets) used to indicate
// generated symbols.
std::string Escape(const std::string &str);

}  // namespace fst

#endif  // FST_UTIL_STRING_STRINGUTIL_H_
