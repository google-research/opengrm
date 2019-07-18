
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
// state-weights.h
//
// Routines to manipulate weights associated with SFST states.

#ifndef SFST_STATE_WEIGHTS_H_
#define SFST_STATE_WEIGHTS_H_

#include <fstream>
#include <iostream>
#include <ostream>

#include <fst/fst.h>
#include <fst/matcher.h>
#include <fst/signed-log-weight.h>
#include <fst/vector-fst.h>
#include <sfst/sfst.h>

namespace sfst {

namespace internal {

//
// Weight vector utilities
//

// Useful with STL. 'delta' is the FST weight approximation delta
// and approx_zero a value considered approximately Zero(). If
// the latter is unset, a default is used.
template <class Weight>
class ApproxEqualPred {
 public:
  explicit ApproxEqualPred(float delta = fst::kDelta,
                           Weight approx_zero = Weight::NoWeight())
      : delta_(delta),
        approx_zero_(approx_zero.Member() ? to_log_(approx_zero) :
                     kApproxZeroWeight) {}

  bool operator()(const Weight &w1, const Weight &w2) const {
    return fst::ApproxEqual(w1, w2, delta_) ||
        (Less(to_log_(w1), approx_zero_) &&
         Less(to_log_(w2), approx_zero_));
  }

 private:
  fst::WeightConvert<Weight, fst::Log64Weight> to_log_;
  float delta_;
  fst::Log64Weight approx_zero_;
};

// Specialization for signed log weight.
template <typename T>
class ApproxEqualPred<fst::SignedLogWeightTpl<T>> {
 public:
  using Weight = fst::SignedLogWeightTpl<T>;
  explicit ApproxEqualPred(float delta = fst::kDelta,
                           Weight approx_zero = Weight::NoWeight())
      : delta_(delta),
        approx_zero_(approx_zero.Member() ? approx_zero :
                     Weight(1.0, kApproxZeroWeight.Value())) {}

  bool operator()(const Weight &w1, const Weight &w2) const {
    return fst::ApproxEqual(w1, w2, delta_) ||
        (Less(w1.Value2(), approx_zero_.Value2()) &&
         Less(w2.Value2(), approx_zero_.Value2()));
  }

 private:
  float delta_;
  Weight approx_zero_;
};

}  // namespace internal

// Normalizes a weight vector.
template <class Weight>
void NormWeights(std::vector<Weight> *weights) {
  namespace f = fst;
  f::WeightConvert<f::Log64Weight, Weight> from_log;
  f::WeightConvert<Weight, f::Log64Weight> to_log;

  f::Adder<f::Log64Weight> sum;
  for (size_t i = 0; i < weights->size(); ++i)
    sum.Add(to_log((*weights)[i]));
  for (size_t i = 0; i < weights->size(); ++i)
    (*weights)[i] = Divide((*weights)[i], from_log(sum.Sum()));
}

// Checks two vectors contain approximately the same weights.
// See ApproxEqualPred for the approximation argument explanation.
template <class Weight>
bool ApproxEqualWeights(const std::vector<Weight> &weights1,
                        const std::vector<Weight> &weights2,
                        float delta = fst::kDelta,
                        Weight approx_zero = Weight::NoWeight()) {
  namespace f = fst;
  internal::ApproxEqualPred<Weight> aepred(delta, approx_zero);
  if (weights1.size() != weights2.size()) return false;
  return std::equal(weights1.begin(), weights1.end(),
                    weights2.begin(), aepred);
}

// Writes weights to stream.
template <class Weight>
void WriteWeights(std::ostream &strm,
                  const std::vector<Weight> &weights) {
  strm.precision(9);
  for (size_t s = 0; s < weights.size(); ++s)
    strm << s << "\t" << weights[s] << "\n";
}

// Writes weights to a file. Returns true on success.
template <class Weight>
    bool WriteWeights(const std::string &filename,
                      const std::vector<Weight> &weights) {
  std::ofstream ostrm;
  if (!filename.empty()) {
    ostrm.open(filename);
    if (!ostrm.good()) {
      LOG(ERROR) << "WriteWeights: Can't open file: " << filename;
      return false;
    }
  }
  std::ostream &strm = ostrm.is_open() ? ostrm : std::cout;
  WriteWeights(strm, weights);
  if (strm.fail()) {
    LOG(ERROR) << "WritePotentials: Write failed: "
               << (filename.empty() ? "standard output" : filename);
    return false;
  }
  return true;
}

//
// State weight utilities
//


// Converts between state weights that exclude the incoming failure mass
// (e.g. as produced by PhiStationaryDistrib) to those that include it
// (e.g. as produced by PhiShortestDistance). The 'fail_arc' bool
// determines in the incoming failure arc weight is included. Assumes
// (but does not fully check) that the input has the canonical topology
// (see canonical.h).
template <class Arc>
void SumStateWeights(const fst::Fst<Arc> &fst,
                     std::vector<typename Arc::Weight> *weights,
                     typename Arc::Label phi_label, bool fail_arc) {
  namespace f = fst;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  using Matr = f::ExplicitMatcher<f::Matcher<f::Fst<Arc>>>;

  f::WeightConvert<f::Log64Weight, Weight> from_log;
  f::WeightConvert<Weight, f::Log64Weight> to_log;

  if (phi_label == f::kNoLabel)
    return;

  std::vector<StateId> top_order;
  bool acyclic = PhiTopOrder(fst, phi_label, &top_order);
  if (!acyclic) {
    FSTERROR() << "SumStateWeights: FST not canonical (phi-cyclic)";
    return;
  }
  Matr matcher(fst, f::MATCH_INPUT);
  for (StateId i = 0; i < top_order.size(); ++i) {
    StateId s = top_order[i];   // ith state in phi-top order
    matcher.SetState(s);
    if (matcher.Find(phi_label)) {
      const Arc &arc = matcher.Value();
      f::Log64Weight w1 = to_log((*weights)[arc.nextstate]);
      f::Log64Weight w2 = to_log(fail_arc ? Times((*weights)[s], arc.weight) :
                                 (*weights)[s]);
      (*weights)[arc.nextstate] = from_log(Plus(w1, w2));
    }
  }
}

// Converts between state weights that include the incoming failure mass
// (e.g. as produced by PhiStationaryDistrib) to those that exclude it
// (e.g. as produced by PhiStationaryDistrib). The 'fail_arc' bool
// determines in the incoming failure arc weight is included. Assumes
// (but does not fully check) that the input has the canonical topology
// (see canonical.h).
template <class Arc>
void DiffStateWeights(const fst::Fst<Arc> &fst,
                      std::vector<typename Arc::Weight> *weights,
                      typename Arc::Label phi_label, bool fail_arc) {
  namespace f = fst;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  using Matr = f::ExplicitMatcher<f::Matcher<f::Fst<Arc>>>;

  f::WeightConvert<f::Log64Weight, Weight> from_log;
  f::WeightConvert<Weight, f::Log64Weight> to_log;

  if (phi_label == f::kNoLabel)
    return;

  std::vector<StateId> top_order;
  bool acyclic = PhiTopOrder(fst, phi_label, &top_order);
  if (!acyclic) {
    FSTERROR() << "DiffStateWeights: FST not canonical (phi-cyclic)";
    return;
  }
  Matr matcher(fst, f::MATCH_INPUT);
  for (StateId i = top_order.size() - 1 ; i >= 0; --i) {
    StateId s = top_order[i];   // ith state in reverse phi-top order
    matcher.SetState(s);
    if (matcher.Find(phi_label)) {
      const Arc &arc = matcher.Value();
      f::Log64Weight w1 = to_log((*weights)[arc.nextstate]);
      f::Log64Weight w2 = to_log(fail_arc ? Times((*weights)[s], arc.weight) :
                                 (*weights)[s]);
      (*weights)[arc.nextstate] = Less(w2, w1) ?
          from_log(Minus(w1, w2)) : Weight::Zero();
    }
  }
}

// Sums arcs (including superfinal) leaving state. Assumes
// (but does not fully check) that the input has the canonical topology
// (see canonical.h).
template <class Arc>
void SumStates(const fst::Fst<Arc> &fst, typename Arc::Label phi_label,
               std::vector<typename Arc::Weight> *weights) {
  namespace f = fst;
  using ArcItr = f::ArcIterator<f::Fst<Arc>>;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;
  f::WeightConvert<f::Log64Weight, Weight> from_log;
  f::WeightConvert<Weight, f::Log64Weight> to_log;
  weights->clear();

  std::vector<StateId> top_order;
  bool acyclic = PhiTopOrder(fst, phi_label, &top_order);
  if (!acyclic) {
    FSTERROR() << "SumStates: FST not canonical (phi-cyclic)";
    return;
  }
  weights->resize(top_order.size(), Weight::Zero());

  for (StateId i = top_order.size() - 1 ; i >= 0; --i) {
    StateId s = top_order[i];   // ith state in reverse phi-top order
    f::Adder<f::Log64Weight> ssum(to_log(fst.Final(s)));
    for (ArcItr aiter(fst, s); !aiter.Done(); aiter.Next()) {
      const Arc &arc = aiter.Value();
      ssum.Add(to_log(arc.weight));
    }
    (*weights)[s] = from_log(ssum.Sum());
  }
}


}  // namespace sfst

#endif  // SFST_STATE_WEIGHTS_H_
