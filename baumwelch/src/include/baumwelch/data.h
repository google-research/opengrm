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

#ifndef BAUMWELCH_DATA_H_
#define BAUMWELCH_DATA_H_

#include <string>

#include <fst/extensions/far/far.h>
#include <fst/fst-decl.h>

namespace fst {
namespace internal {

// Data objects hold two arguments representing input and output. The input may
// be a FAR reader or a language model; the output is assumed to be a FAR
// reader. They have the following interface:
//
// template <class Arc>
// class Data {
//  public:
//   // Some sensible constructor here.
//
//   const Fst<Arc> &GetInput() const;
//
//   const Fst<Arc> &GetOutput() const;
//
//   // Returns some sensible key.
//   const std::string &GetKey() const
//
//   void Reset();
//
//   void Next();
//
//   // Should fail if either are done.
//   bool Done() const;
// };

// Data object for pairs of FARs.
template <class Arc>
class PairedData {
 public:
  PairedData(FarReader<Arc> *plaintext, FarReader<Arc> *ciphertext) :
    plaintext_(plaintext),
    ciphertext_(ciphertext) {}

  const Fst<Arc> &GetInput() const { return *plaintext_->GetFst(); }

  const Fst<Arc> &GetOutput() const { return *ciphertext_->GetFst(); }

  // This is recomputed on every call.
  const std::string &GetKey() const {
    key_ = plaintext_->GetKey() + "_" + ciphertext_->GetKey();
    return key_;
  }

  void Reset() {
    plaintext_->Reset();
    ciphertext_->Reset();
  }

  void Next() {
    plaintext_->Next();
    ciphertext_->Next();
  }

  bool Done() const { return plaintext_->Done() || ciphertext_->Done(); }

 private:
  FarReader<Arc> *plaintext_;
  FarReader<Arc> *ciphertext_;
  mutable std::string key_;
};

// Data object with an input FST (usually a language model) and an output
// FAR.
template <class Arc>
class DeciphermentData {
 public:
  DeciphermentData(const Fst<Arc> &plaintext, FarReader<Arc> *ciphertext) :
    plaintext_(plaintext),
    ciphertext_(ciphertext) {}

  const Fst<Arc> &GetInput() const { return plaintext_; }

  const Fst<Arc> &GetOutput() const { return *ciphertext_->GetFst(); }

  const std::string &GetKey() const { return ciphertext_->GetKey(); }

  bool Done() const { return ciphertext_->Done(); }

  void Next() { ciphertext_->Next(); }

  void Reset() { ciphertext_->Reset(); }

 private:
  const Fst<Arc> &plaintext_;
  FarReader<Arc> *ciphertext_;
};

}  // namespace internal
}  // namespace fst

#endif  // BAUMWELCH_DATA_H_

