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

void Decode(FarReaderClass &input, FarReaderClass &output,
            const FstClass &model, FarWriterClass &hypotext) {
  if (!internal::ArcTypesMatch(input, model, "Decode") ||
      !internal::ArcTypesMatch(output, model, "Decode") ||
      !internal::ArcTypesMatch(hypotext, model, "Decode")) {
    return;
  }
  BaumWelchDecodeArgs args{input, output, model, hypotext};
  Apply<Operation<BaumWelchDecodeArgs>>("Decode", model.ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(Decode, BaumWelchDecodeArgs);

}  // namespace script
}  // namespace fst

