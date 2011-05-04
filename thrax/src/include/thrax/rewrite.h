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
// Copyright 2005-2011 Google, Inc.
// Author: ttai@google.com (Terry Tai)
//
// Wrapper for the rewrite function, where we have two FSTs and rewrite the
// the first into the second.  This function creates only on-the-fly
// transducers.

#ifndef THRAX_REWRITE_H_
#define THRAX_REWRITE_H_

#include <vector>
using std::vector;

#include <fst/compat.h>
#include <thrax/compat/compat.h>
#include <fst/types.h>
#include <fst/concat.h>
#include <fst/fst.h>
#include <fst/map.h>
#include <fst/properties.h>
#include <fst/vector-fst.h>
#include <thrax/function.h>
#include <thrax/datatype.h>
#include <thrax/compat/stlfunctions.h>
#include <thrax/compat/registry.h>

namespace thrax {
namespace function {

// A Mapper that turns either all input or all output labels into epsilons.
template <typename Arc>
class EpsilonMapper {
 public:
  enum Type {
    EPSILON_INPUTS,
    EPSILON_OUTPUTS,
  };

  explicit EpsilonMapper(Type type)
      : type_(type) {}

  Arc operator()(const Arc& arc) {
    if (type_ == EPSILON_INPUTS)
      return Arc(0, arc.olabel, arc.weight, arc.nextstate);
    else /* if (type_ == EPSILON_OUTPUTS) */
      return Arc(arc.ilabel, 0, arc.weight, arc.nextstate);
  }

  fst::MapFinalAction FinalAction() const {
    return fst::MAP_NO_SUPERFINAL;
  }

  fst::MapSymbolsAction InputSymbolsAction() const {
    return type_ == EPSILON_INPUTS ? fst::MAP_CLEAR_SYMBOLS :
        fst::MAP_COPY_SYMBOLS;
  }

  fst::MapSymbolsAction OutputSymbolsAction() const {
    return type_ == EPSILON_OUTPUTS ? fst::MAP_CLEAR_SYMBOLS :
        fst::MAP_COPY_SYMBOLS;
  }

  uint64 Properties(uint64 in_properties) const {
    uint64 out_properties = 0;
    if (type_ == EPSILON_INPUTS) {
      out_properties |= fst::kILabelInvariantProperties & in_properties;
      out_properties |= fst::kILabelSorted;
    } else /* if (type_ == EPSILON_OUTPUTS) */ {
      out_properties |= fst::kOLabelInvariantProperties & in_properties;
      out_properties |= fst::kOLabelSorted;
    }
    return out_properties;
  }

 private:
  Type type_;
};

template <typename Arc>
class Rewrite : public BinaryFstFunction<Arc> {
 public:
  typedef fst::Fst<Arc> Transducer;

  Rewrite() {}
  virtual ~Rewrite() {}

 protected:
  virtual Transducer* BinaryFstExecute(const Transducer& left,
                                       const Transducer& right,
                                       const vector<DataType*>& args) {
    if (args.size() != 2) {
      cout << "Rewrite: Expected 2 arguments but got " << args.size() << endl;
      return NULL;
    }

    fst::MapFst<Arc, Arc, EpsilonMapper<Arc> > left_rmep(
        left, EpsilonMapper<Arc>(EpsilonMapper<Arc>::EPSILON_OUTPUTS));
    fst::MapFst<Arc, Arc, EpsilonMapper<Arc> > right_rmep(
        right, EpsilonMapper<Arc>(EpsilonMapper<Arc>::EPSILON_INPUTS));

    return new fst::ConcatFst<Arc>(left_rmep, right_rmep);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(Rewrite<Arc>);
};

}  // namespace function
}  // namespace thrax

#endif  // THRAX_REWRITE_H_
