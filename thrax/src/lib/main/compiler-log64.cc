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
// Compiling a single arc specialization of CompileGrammar takes 60 seconds,
// so we split each arc type into a separate file.

#include <thrax/compiler.h>

namespace thrax {

template bool CompileGrammar<::fst::Log64Arc>(const std::string&,
                                                  const std::string&, bool,
                                                  bool);

}  // namespace thrax
