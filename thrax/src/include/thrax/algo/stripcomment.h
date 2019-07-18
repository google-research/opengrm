#ifndef FST_UTIL_STRING_STRIPCOMMENT_H_
#define FST_UTIL_STRING_STRIPCOMMENT_H_

#include <string>

// Defines comment syntax for string files.
//
// The comment character is '#', and has scope until the end of the line. Any
// preceding whitespace before a comment is ignored.
//
// To use the '#' literal (i.e., to ensure it is not interpreted as the start of
// a comment) escape it with '\'; the escaping '\' in "\#" also removed.
//
// TODO(rws,kbg): Merge stringfile functionality across Pynini and Thrax.

namespace fst {

std::string StripCommentAndRemoveEscape(const std::string &line);

}  // namespace fst

#endif  // FST_UTIL_STRING_STRIPCOMMENT_H_
