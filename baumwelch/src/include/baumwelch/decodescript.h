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

#ifndef BAUMWELCH_DECODESCRIPT_H_
#define BAUMWELCH_DECODESCRIPT_H_

#include <fst/extensions/far/far-class.h>
#include <fst/script/fst-class.h>
#include <baumwelch/decode.h>

namespace fst {
namespace script {

using DecodeBaumWelchArgs = std::tuple<FarReaderClass *, FarReaderClass *,
                                       const FstClass &, FarWriterClass *>;

template <class Arc>
void DecodeBaumWelch(DecodeBaumWelchArgs *args) {
  FarReader<Arc> *input = std::get<0>(*args)->GetFarReader<Arc>();
  FarReader<Arc> *output = std::get<1>(*args)->GetFarReader<Arc>();
  const Fst<Arc> &model = *(std::get<2>(*args).GetFst<Arc>());
  FarWriter<Arc> *hypotext = std::get<3>(*args)->GetFarWriter<Arc>();
  DecodeBaumWelch(input, output, model, hypotext);
}

void DecodeBaumWelch(FarReaderClass *input, FarReaderClass *output,
                     const FstClass &model, FarWriterClass *hypotext);

}  // namespace script
}  // namespace fst

#endif  // BAUMWELCH_DECODESCRIPT_H_

