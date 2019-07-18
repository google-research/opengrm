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
// Copyright 2017 and onwards Google, Inc.

#include <baumwelch/getters.h>

namespace fst {
namespace script {

bool GetExpectationTableType(const std::string &str,
                             ExpectationTableType *etype) {
  if (str == "global") {
    *etype = GLOBAL;
  } else if (str == "state") {
    *etype = STATE;
  } else if (str == "ilabel") {
    *etype = ILABEL;
  } else if (str == "state_ilabel") {
    *etype = STATE_ILABEL;
  } else {
    return false;
  }
  return true;
}

}  // namespace script
}  // namespace fst

