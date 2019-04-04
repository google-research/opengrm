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

#include <baumwelch/decodescript.h>
#include <fst/script/script-impl.h>

namespace fst {
namespace script {

// 1: Pair construction.
void DecodeBaumWelch(FarReaderClass *plaintext, FarReaderClass *ciphertext,
                     const FstClass &channel, FarWriterClass *hypotext) {
  if (!internal::ArcTypesMatch(*plaintext, channel, "DecodeBaumWelch") ||
      !internal::ArcTypesMatch(*ciphertext, channel, "DecodeBaumWelch") ||
      !internal::ArcTypesMatch(*hypotext, channel, "DecodeBaumWelch")) {
    return;
  }
  DecodeBaumWelchArgs1 args(plaintext, ciphertext, channel, hypotext);
  Apply<Operation<DecodeBaumWelchArgs1>>("DecodeBaumWelch", channel.ArcType(),
                                         &args);
}

// 2: Decipherment construction.
void DecodeBaumWelch(const FstClass &plaintext, FarReaderClass *ciphertext,
                     const FstClass &channel, FarWriterClass *hypotext) {
  if (!internal::ArcTypesMatch(plaintext, channel, "DecodeBaumWelch") ||
      !internal::ArcTypesMatch(*ciphertext, channel, "DecodeBaumWelch") ||
      !internal::ArcTypesMatch(*hypotext, channel, "DecodeBaumWelch")) {
    return;
  }
  DecodeBaumWelchArgs2 args(plaintext, ciphertext, channel, hypotext);
  Apply<Operation<DecodeBaumWelchArgs2>>("DecodeBaumWelch", channel.ArcType(),
                                         &args);
}

REGISTER_FST_OPERATION(DecodeBaumWelch, StdArc, DecodeBaumWelchArgs1);
REGISTER_FST_OPERATION(DecodeBaumWelch, LogArc, DecodeBaumWelchArgs1);
REGISTER_FST_OPERATION(DecodeBaumWelch, Log64Arc, DecodeBaumWelchArgs1);

REGISTER_FST_OPERATION(DecodeBaumWelch, StdArc, DecodeBaumWelchArgs2);
REGISTER_FST_OPERATION(DecodeBaumWelch, LogArc, DecodeBaumWelchArgs2);
REGISTER_FST_OPERATION(DecodeBaumWelch, Log64Arc, DecodeBaumWelchArgs2);

}  // namespace script
}  // namespace fst

