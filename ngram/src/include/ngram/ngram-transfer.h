// ngram-transfer.h
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
// Copyright 2009-2013 Brian Roark and Google, Inc.
// Authors: roarkbr@gmail.com  (Brian Roark)
//          allauzen@google.com (Cyril Allauzen)
//          riley@google.com (Michael Riley)
//
// \file
// Class for transferring n-grams across multiple parts split by context.

#ifndef NGRAM_NGRAM_TRANSFER_H__
#define NGRAM_NGRAM_TRANSFER_H__

#include <string>
#include <vector>

#include <fst/fst.h>
#include <fst/mutable-fst.h>
#include <fst/vector-fst.h>
#include <ngram/ngram-context.h>
#include <ngram/ngram-mutable-model.h>

namespace ngram {

using fst::StdArc;
using fst::StdFst;
using fst::StdMutableFst;
using fst::StdVectorFst;

class NGramTransfer {
 public:
  typedef StdArc::StateId StateId;
  typedef StdArc::Label Label;
  typedef StdArc::Weight Weight;

  // Ctr for transfer from source FST(s) to this ctr FST.
  NGramTransfer(StdMutableFst *fst,
                const string &context_pattern,
                Label backoff_label = 0,
                double norm_eps = kNormEps)
      : transfer_from_(true) {
    InitDest(fst, context_pattern, backoff_label, norm_eps);
  }

  // Ctr for transfer from this ctr FST to destination FST(s).
  NGramTransfer(const StdFst &fst,
                const string &context_pattern,
                Label backoff_label = 0,
                double norm_eps = kNormEps)
      : transfer_from_(false) {
    InitSrc(fst, context_pattern, backoff_label, norm_eps);
  }

  ~NGramTransfer() {
    if (transfer_from_) {
      DeleteDest();
    } else {
      DeleteSrc();
    }
  }

  // Transfer to ctr FST from this arg FST
  void TransferNGramsFrom(const StdFst &fst, const string &context_pn,
                          bool normalize = false) {
    if (!transfer_from_)
      LOG(FATAL) <<
          "NGramTransfer::NGramTransferFrom: argument FST is not mutable";
    InitSrc(fst, context_pn, dest_model_->BackoffLabel(),
            dest_model_->NormEps());
    TransferNGrams(normalize);
    DeleteSrc();
  }

  // Transfer from ctr FST to this arg FST
  void TransferNGramsTo(StdMutableFst *fst, const string &context_pn,
                        bool normalize = false) {
    if (transfer_from_)
      LOG(FATAL) <<
          "NGramTransfer::NGramTransferTo: constructor FST should not be mutable";
    InitDest(fst, context_pn, src_model_->BackoffLabel(),
             src_model_->NormEps());
    TransferNGrams(normalize);
    DeleteDest();
  }

 private:
  void TransferNGrams(bool normalize) const;

  StateId FindNextState(StateId s, Label label) const;

  void InitSrc(const StdFst &fst, const string &context_pattern,
               Label backoff_label, double norm_eps) {
    src_fst_ = fst.Copy();
    src_matcher_ = new Matcher<StdFst>(*src_fst_, MATCH_INPUT);
    src_model_ = new NGramModel(*src_fst_, backoff_label, norm_eps, true);
    src_context_ = new NGramContext(context_pattern, src_model_->HiOrder());
  }

  void InitDest(StdMutableFst *fst, const string &context_pattern,
                Label backoff_label, double norm_eps) {
    dest_fst_ = fst;
    dest_model_ = new NGramMutableModel(dest_fst_, backoff_label, norm_eps);
    dest_context_ = new NGramContext(context_pattern, dest_model_->HiOrder());
  }

  void DeleteSrc() {
    delete src_model_;
    delete src_matcher_;
    delete src_fst_;
    delete src_context_;
  }

  void DeleteDest() {
    delete dest_model_;
    delete dest_context_;
  }

 private:
  const StdFst *src_fst_;
  Matcher<StdFst> *src_matcher_;
  NGramModel *src_model_;
  NGramContext *src_context_;

  StdMutableFst *dest_fst_;
  NGramMutableModel *dest_model_;
  NGramContext *dest_context_;

  NGramContext *context_;
  bool transfer_from_;   // transfer from arg FST to ctr FST?
};

}  // namespace ngram

#endif  // NGRAM_NGRAM_TRANSFER_H__
