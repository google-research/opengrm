
// Licensed under the Apache License, Version 2.0 (the 'License');
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an 'AS IS' BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2005-2016 Brian Roark and Google, Inc.
// Class for transferring n-grams across multiple parts split by context.

#ifndef NGRAM_NGRAM_TRANSFER_H_
#define NGRAM_NGRAM_TRANSFER_H_

#include <string>
#include <vector>

#include <fst/fst.h>
#include <fst/mutable-fst.h>
#include <fst/vector-fst.h>
#include <ngram/ngram-context.h>
#include <ngram/ngram-model.h>
#include <ngram/ngram-mutable-model.h>
#include <ngram/util.h>

namespace ngram {

using fst::StdArc;
using fst::Fst;
using fst::MutableFst;
using fst::VectorFst;

template <class Arc>
class NGramTransfer {
 public:
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;

  // Ctr for transfer from source FST(s) to this ctr FST.
  NGramTransfer(MutableFst<Arc> *fst, const string &context_pattern,
                Label backoff_label = 0, double norm_eps = kNormEps)
      : transfer_from_(true), error_(false) {
    InitDest(fst, context_pattern, backoff_label, norm_eps);
  }

  // Ctr for transfer from this ctr FST to destination FST(s).
  NGramTransfer(const Fst<Arc> &fst, const string &context_pattern,
                Label backoff_label = 0, double norm_eps = kNormEps)
      : transfer_from_(false), error_(false) {
    InitSrc(fst, context_pattern, backoff_label, norm_eps);
  }

  // Transfer to ctr FST from this arg FST
  bool TransferNGramsFrom(const Fst<Arc> &fst, const string &context_pn) {
    if (Error()) return false;
    if (!transfer_from_) {
      NGRAMERROR()
          << "NGramTransfer::NGramTransferFrom: argument FST is not mutable";
      SetError();
      return false;
    }
    InitSrc(fst, context_pn, dest_model_->BackoffLabel(),
            dest_model_->NormEps());
    if (!TransferNGrams()) SetError();
    return !Error();
  }

  // Transfer from ctr FST to this arg FST
  bool TransferNGramsTo(MutableFst<Arc> *fst, const string &context_pn) {
    if (Error()) return false;
    if (transfer_from_) {
      NGRAMERROR() << "NGramTransfer::NGramTransferTo: constructor FST should "
                      "not be mutable";
      SetError();
      return false;
    }
    InitDest(fst, context_pn, src_model_->BackoffLabel(),
             src_model_->NormEps());
    if (!TransferNGrams()) SetError();
    return !Error();
  }

  // Normalizes model after all transfer has taken place.
  bool TransferNormalize() {
    if (!Error()) {
      dest_model_->RecalcBackoff();
      if (dest_model_->Error()) SetError();
    }
    return !Error();
  }

 private:
  bool TransferNGrams() const;

  StateId FindNextState(StateId s, Label label) const;

  void InitSrc(const Fst<Arc> &fst, const string &context_pattern,
               Label backoff_label, double norm_eps) {
    src_fst_.reset(fst.Copy());
    src_matcher_.reset(new Matcher<Fst<Arc>>(*src_fst_, MATCH_INPUT));
    src_model_.reset(
        new NGramModel<Arc>(*src_fst_, backoff_label, norm_eps, true));
    src_context_.reset(
        new NGramContext(context_pattern, src_model_->HiOrder()));
    if (src_model_->Error()) SetError();
  }

  void InitDest(MutableFst<Arc> *fst, const string &context_pattern,
                Label backoff_label, double norm_eps) {
    dest_fst_ = fst;
    dest_model_.reset(
        new NGramMutableModel<Arc>(dest_fst_, backoff_label, norm_eps));
    dest_context_.reset(
        new NGramContext(context_pattern, dest_model_->HiOrder()));
    if (dest_model_->Error()) SetError();
  }

  // Returns true if model in a bad state/not a proper LM.
  bool Error() const { return error_; }

 protected:
  void SetError() { error_ = true; }

 private:
  std::unique_ptr<const Fst<Arc>> src_fst_;
  std::unique_ptr<Matcher<Fst<Arc>>> src_matcher_;
  std::unique_ptr<NGramModel<Arc>> src_model_;
  std::unique_ptr<NGramContext> src_context_;

  MutableFst<Arc> *dest_fst_;
  std::unique_ptr<NGramMutableModel<Arc>> dest_model_;
  std::unique_ptr<NGramContext> dest_context_;

  std::unique_ptr<NGramContext> context_;
  bool transfer_from_;  // transfer from arg FST to ctr FST?
  bool error_;
};

template <typename Arc>
typename Arc::StateId NGramTransfer<Arc>::FindNextState(StateId s,
                                                        Label label) const {
  Matcher<Fst<Arc>> matcher(*dest_fst_, MATCH_INPUT);
  matcher.SetState(s);
  StateId bs = s;
  Label find_backoff_label =
      src_model_->BackoffLabel() ? src_model_->BackoffLabel() : kNoLabel;
  bool found = false;
  do {
    if (matcher.Find(find_backoff_label))
      bs = matcher.Value().nextstate;
    else
      return bs;
    matcher.SetState(bs);
    found = matcher.Find(label);
  } while (!found);
  return matcher.Value().nextstate;
}

template <typename Arc>
bool NGramTransfer<Arc>::TransferNGrams() const {
  if (Error()) return false;
  std::vector<StateId> states(src_model_->NumStates(), kNoStateId);
  states[src_fst_->Start()] = dest_fst_->Start();
  if (src_model_->UnigramState() >= 0) {  // src_model_ is not a unigram model.
    if (dest_model_->UnigramState() < 0) {
      NGRAMERROR() << "destination model is a unigram but source model is not";
      return false;
    }
    states[src_model_->UnigramState()] = dest_model_->UnigramState();
  } else {  // src_model_ is a unigram model, so should dest_model_ be.
    if (dest_model_->UnigramState() != src_model_->UnigramState()) {
      NGRAMERROR() << "destination model is not a unigram but source model is";
      return false;
    }
  }

  for (int order = 1; order <= src_model_->HiOrder(); ++order) {
    for (StateId s = 0; s < src_model_->NumStates(); ++s) {
      if (src_model_->StateOrder(s) != order) continue;
      if (states[s] == kNoStateId) continue;
      StateId sp = states[s];

      // (1) if both ascending, set states[d] = dp
      src_matcher_->SetState(s);
      for (ArcIterator<Fst<Arc>> aiter(*dest_fst_, sp); !aiter.Done();
           aiter.Next()) {
        const Arc &arc = aiter.Value();
        if (arc.ilabel == dest_model_->BackoffLabel()) continue;
        if (dest_model_->StateOrder(arc.nextstate) <=
            dest_model_->StateOrder(sp)) {
          continue;
        }
        if (!src_matcher_->Find(arc.ilabel)) continue;
        if (dest_model_->StateOrder(arc.nextstate) !=
            src_model_->StateOrder(src_matcher_->Value().nextstate)) {
          continue;
        }
        if (states[src_matcher_->Value().nextstate] != kNoStateId) {
          NGRAMERROR() << "State " << src_matcher_->Value().nextstate
                       << " value already set to "
                       << states[src_matcher_->Value().nextstate];
          return false;
        }
        states[src_matcher_->Value().nextstate] = arc.nextstate;
      }

      // (2) if strictly in context in the reference, do the transfer
      if (!src_context_->HasContext(src_model_->StateNGram(s), false)) {
        continue;
      }

      std::map<Label, Arc> arcs;
      for (ArcIterator<Fst<Arc>> aiter(*dest_fst_, sp); !aiter.Done();
           aiter.Next()) {
        const Arc &arc = aiter.Value();
        arcs[arc.ilabel] = arc;
      }

      // If loosely in context for the destination, missing arcs and finality
      // need to be added.
      bool add_missing =
          dest_context_->HasContext(src_model_->StateNGram(s), true);

      for (ArcIterator<Fst<Arc>> aiter(*src_fst_, s); !aiter.Done();
           aiter.Next()) {
        const Arc &arc = aiter.Value();
        auto iter = arcs.find(arc.ilabel);
        if (iter != arcs.end()) {
          iter->second.weight = arc.weight;
        } else if (add_missing) {
          arcs[arc.ilabel] = Arc(arc.ilabel, arc.olabel, arc.weight,
                                 FindNextState(sp, arc.ilabel));
        }
      }

      dest_fst_->DeleteArcs(sp);
      for (auto iter = arcs.begin(); iter != arcs.end(); ++iter) {
        dest_fst_->AddArc(sp, iter->second);
      }
      if (NGramModel<Arc>::ScalarValue(src_fst_->Final(s)) !=
              NGramModel<Arc>::ScalarValue(Weight::Zero()) &&
          (add_missing ||
           NGramModel<Arc>::ScalarValue(dest_fst_->Final(sp)) !=
               NGramModel<Arc>::ScalarValue(Weight::Zero()))) {
        dest_fst_->SetFinal(sp, src_fst_->Final(s));
      }
    }
  }
  return true;
}

}  // namespace ngram

#endif  // NGRAM_NGRAM_TRANSFER_H_
