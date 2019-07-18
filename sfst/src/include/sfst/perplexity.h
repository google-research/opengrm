
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
// Copyright 2018 Google, Inc.
// perplextiy.h
//
// Computes perplexity for a stochastic FST.

#ifndef SFST_PERPLEXITY_H_
#define SFST_PERPLEXITY_H_

#include <stddef.h>

#include <cmath>

#include <fst/log.h>
#include <fst/intersect.h>
#include <fst/shortest-distance.h>
#include <sfst/normalize.h>
#include <sfst/rmphi.h>

namespace sfst {

// Computes the perplexity for a stochastic FST. The SFST must be a
// normalized stochastic FST. Each evaulated FST must be a single path
// (topologically sorted). Both conditions are checked (and reported
// via Apply() return value).
template <class Arc>
class Perplexity {
 public:
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::Label Label;
  typedef fst::SignedLog64Weight SLWeight;
  typedef fst::SignedLog64Arc SLArc;
  typedef fst::WeightConvertMapper<Arc, SLArc> WCM;

  explicit Perplexity(const fst::Fst<Arc> &sfst,
                      Label phi_label = fst::kNoLabel,
                      Label unknown_label =  fst::kNoLabel,
                      size_t unknown_class_size = 10000,
                      float delta = fst::kDelta,
                      float shortest_delta = fst::kShortestDelta);

  // Computes perplexity on next input. Returns false on error.
  bool Apply(const  fst::Fst<Arc> &fst) {
    namespace f = fst;
    if (error_)
      return false;
    if (!fst.Properties(f::kString, true)) {
      LOG(ERROR) << "Perplexity::Apply: input is not a single path "
                 << "(topologically sorted).";
      return false;
    }

    f::VectorFst<SLArc> wc_fst, res_fst;
    PrepareFst(fst, &wc_fst);
    f::Intersect(wc_fst, slfst_, &res_fst);
    SLWeight distance = f::ShortestDistance(res_fst, shortest_delta_);
    sent_count_ += 1;
    if (distance == SLWeight::Zero()) {
      ++skip_count_;
    } else {
      sum_ += distance.Value2().Value();
      word_count_ += fst_word_count_;
      oov_count_ += fst_oov_count_;
    }
    return true;
  }

  // Returns accumulated cross entropy since construction or last reset.
  double GetCrossEntropy() const {
    return sum_;
  }

  // Returns accumulated perplexity since construction or last reset.
  double GetPerplexity() const {
    return exp(sum_/(word_count_ + sent_count_ - skip_count_));
  }

  // Returns total # of sentences since construction or last reset.
  size_t NumSentences() const { return sent_count_; }

  // Returns # of skipped sentences since construction or last reset.
  size_t NumSkipped() const { return skip_count_; }

  // Returns total # of words since construction or last reset (ignoring
  // skipped sentences)
  size_t NumWords() const { return word_count_; }

  // Returns # of OOV words since construction or last reset (ignoring
  // skipped sentences)
  size_t NumOOVs() const { return oov_count_; }


  // Resets perplexity computation.
  void Reset() {
    sum_ = 0.0;
    sent_count_ = 0;
    skip_count_ = 0;
    word_count_ = 0;
    oov_count_ = 0;
  }


 private:
  // Finds in-vocabulary words
  void FindWordSet(const fst::Fst<Arc> &sfst) {
    namespace f = fst;
    for (f::StateIterator<f::Fst<Arc>> siter(sfst);
         !siter.Done();
         siter.Next()) {
      StateId s = siter.Value();
      for (f::ArcIterator<f::Fst<Arc>> aiter(sfst, s);
           !aiter.Done();
           aiter.Next()) {
        const Arc &arc = aiter.Value();
        if (arc.ilabel != phi_label_ && arc.ilabel != 0)
          word_set_.insert(arc.ilabel);
      }
    }
  }

  // Converts FST to signed-log semiring and converts OOVs to unknown_label
  // when defined.
  void PrepareFst(const fst::Fst<Arc> &ifst,
                  fst::MutableFst<SLArc> *ofst);

  Label phi_label_;
  Label unknown_label_;
  size_t unknown_class_size_;
  float delta_;
  float shortest_delta_;
  bool error_;
  fst::VectorFst<SLArc> slfst_;
  double sum_;
  size_t sent_count_;
  size_t skip_count_;
  size_t word_count_;
  size_t oov_count_;
  size_t fst_word_count_;
  size_t fst_oov_count_;
  std::unordered_set<Label> word_set_;

  Perplexity(const Perplexity &) = delete;
  Perplexity &operator=(const Perplexity &) = delete;
};

// Sets up perplexity computation for a stochastic FST.
template <class Arc>
Perplexity<Arc>::Perplexity(const fst::Fst<Arc> &sfst, Label phi_label,
                            Label unknown_label, size_t unknown_class_size,
                            float delta, float shortest_delta)
    : phi_label_(phi_label),
      unknown_label_(unknown_label),
      unknown_class_size_(unknown_class_size),
      delta_(delta),
      shortest_delta_(shortest_delta),
      error_(false),
      sum_(0.0),
      sent_count_(0),
      skip_count_(0),
      word_count_(0),
      oov_count_(0) {
  namespace f = fst;
  if (!IsNormalized(sfst, phi_label, delta)) {
    LOG(ERROR) << "Perplexity: input is not a normalized stochastic FST";
    error_ = true;
  }

  if (unknown_label_ != f::kNoLabel)
    FindWordSet(sfst);
  SLRmPhi(sfst, &slfst_, phi_label);
}

// Converts FST to signed-log semiring and converts OOVs to unknown_label
// when defined.
template <class Arc>
void Perplexity<Arc>::PrepareFst(const fst::Fst<Arc> &ifst,
                                 fst::MutableFst<SLArc> *ofst) {
  namespace f = fst;
  SLWeight unknown_class_weight(1.0, -log(1.0/unknown_class_size_));
  WCM wc_mapper;
  f::ArcMap(ifst, ofst, wc_mapper);
  fst_word_count_ = 0;
  fst_oov_count_ = 0;

  for (StateId s = 0; s < ofst->NumStates(); ++s) {
    for (f::MutableArcIterator<f::MutableFst<SLArc> > aiter(ofst, s);
         !aiter.Done();
         aiter.Next()) {
      SLArc arc = aiter.Value();
      if (arc.ilabel != 0) {
        ++fst_word_count_;
        if (unknown_label_ != f::kNoLabel &&
            (arc.ilabel == unknown_label_ ||
             word_set_.count(arc.ilabel) == 0)) {
          arc.ilabel = arc.olabel = unknown_label_;
          arc.weight = Times(arc.weight, unknown_class_weight);
          aiter.SetValue(arc);
          ++fst_oov_count_;
        }
      }
    }
  }
}

}  // namespace sfst

#endif  // SFST_PERPLEXITY_H_
