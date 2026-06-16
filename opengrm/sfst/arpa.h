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

#ifndef OPENGRM_SFST_ARPA_H_
#define OPENGRM_SFST_ARPA_H_

#include <iostream>
#include <istream>
#include <ostream>

#include "openfst/lib/fst.h"
#include "openfst/lib/mutable-fst.h"

namespace sfst {

// Reads an ARPA format language model from istrm and builds a canonical SFST.
// Returns true on success.
template <class Arc>
void ReadArpa(std::istream& istrm, fst::MutableFst<Arc>* fst);

// Writes a canonical SFST to ostrm in ARPA format.
// Returns true on success.
template <class Arc>
bool WriteArpa(const fst::Fst<Arc>& fst, std::ostream& ostrm);

}  // namespace sfst

#endif  // OPENGRM_SFST_ARPA_H_
