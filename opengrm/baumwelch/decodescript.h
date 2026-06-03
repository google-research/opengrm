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

#ifndef OPENGRM_BAUMWELCH_DECODESCRIPT_H_
#define OPENGRM_BAUMWELCH_DECODESCRIPT_H_

#include <tuple>

#include "openfst/extensions/far/far-class.h"
#include "openfst/extensions/far/far.h"
#include "openfst/lib/encode.h"
#include "openfst/lib/fst.h"
#include "openfst/script/encodemapper-class.h"
#include "openfst/script/fst-class.h"
#include "opengrm/baumwelch/decode.h"

namespace fst {
namespace script {

using BaumWelchDecodeArgs =
    std::tuple<FarReaderClass&, FarReaderClass&, const FstClass&,
               FarWriterClass&, EncodeMapperClass*>;

template <class Arc>
void Decode(BaumWelchDecodeArgs* args) {
  FarReader<Arc>& input = *std::get<0>(*args).GetFarReader<Arc>();
  FarReader<Arc>& output = *std::get<1>(*args).GetFarReader<Arc>();
  const Fst<Arc>& model = *std::get<2>(*args).GetFst<Arc>();
  FarWriter<Arc>& hypotext = *std::get<3>(*args).GetFarWriter<Arc>();
  EncodeMapper<Arc>* mapper =
      std::get<4>(*args) ? std::get<4>(*args)->GetEncodeMapper<Arc>() : nullptr;
  Decode(input, output, model, hypotext, mapper);
}

void Decode(FarReaderClass& input, FarReaderClass& output,
            const FstClass& model, FarWriterClass& hypotext,
            EncodeMapperClass* encoder = nullptr);

}  // namespace script
}  // namespace fst

#endif  // OPENGRM_BAUMWELCH_DECODESCRIPT_H_
