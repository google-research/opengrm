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
#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>
#include <baumwelch/decode.h>

namespace fst {
namespace script {

// 1: Pair construction.

using DecodeBaumWelchArgs1 = std::tuple<FarReaderClass *, FarReaderClass *,
                                        const FstClass &, FarWriterClass *>;

template <class Arc>
void DecodeBaumWelch(DecodeBaumWelchArgs1 *args) {
  FarReader<Arc> *plaintext = std::get<0>(*args)->GetFarReader<Arc>();
  FarReader<Arc> *ciphertext = std::get<1>(*args)->GetFarReader<Arc>();
  const Fst<Arc> &channel = *(std::get<2>(*args).GetFst<Arc>());
  FarWriter<Arc> *hypotext = std::get<3>(*args)->GetFarWriter<Arc>();
  DecodeBaumWelch(plaintext, ciphertext, channel, hypotext);
}

// 2: Decipherment construction.

using DecodeBaumWelchArgs2 = std::tuple<const FstClass &, FarReaderClass *,
                                        const FstClass &, FarWriterClass *>;

template <class Arc>
void DecodeBaumWelch(DecodeBaumWelchArgs2 *args) {
  const Fst<Arc> &plaintext = *(std::get<0>(*args).GetFst<Arc>());
  FarReader<Arc> *ciphertext = std::get<1>(*args)->GetFarReader<Arc>();
  const Fst<Arc> &channel = *(std::get<2>(*args).GetFst<Arc>());
  FarWriter<Arc> *hypotext = std::get<3>(*args)->GetFarWriter<Arc>();
  DecodeBaumWelch(plaintext, ciphertext, channel, hypotext);
}

void DecodeBaumWelch(FarReaderClass *plaintext, FarReaderClass *ciphertext,
                     const FstClass &channel, FarWriterClass *hypotext);

void DecodeBaumWelch(const FstClass &plaintext, FarReaderClass *ciphertext,
                     const FstClass &channel, FarWriterClass *hypotext);

}  // namespace script
}  // namespace fst

#endif  // BAUMWELCH_DECODESCRIPT_H_

