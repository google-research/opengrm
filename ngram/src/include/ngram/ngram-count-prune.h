
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
// Count pruning style model shrinking derived class.

#ifndef NGRAM_NGRAM_COUNT_PRUNE_H_
#define NGRAM_NGRAM_COUNT_PRUNE_H_

#include <ngram/ngram-shrink.h>

namespace ngram {

class NGramCountPrune : public NGramShrink<StdArc> {
 public:
  // Constructs an NGramCountShrink object that count prunes an LM.
  // This version parses a count pattern string.
  // Expected format: "X(+):Y;Z(+):W"  X,Z are n-gram orders
  // '+' optional designation for >= order and Y,W are count minimums
  // ':' delimits prior to count minimum; ';' delimits fields.
  //
  // Example: "2:2;3+:3" signifies:
  //   prune bigrams with count < 2; trigrams and above with count < 3
  NGramCountPrune(StdMutableFst *infst, const string &count_pattern,
                  int shrink_opt = 0, double tot_uni = -1.0,
                  Label backoff_label = 0, double norm_eps = kNormEps,
                  bool check_consistency = false)
      : NGramShrink<StdArc>(infst, shrink_opt < 2 ? shrink_opt : 0, tot_uni,
                            backoff_label, norm_eps, check_consistency) {
    // shrink_opt must be less than 2 for count pruning
    for (int i = 0; i < HiOrder(); ++i)  // initialize minimum values
      count_minimums_.push_back(-StdArc::Weight::Zero().Value());
    if (!count_pattern.empty()) ParseCountMinimums(count_pattern);
  }

  // Constructs an NGramCountShrink object that count prunes an LM.
  // This version is given the count minimums per order.
  NGramCountPrune(StdMutableFst *infst,
                  const std::vector<double> &count_minimums, int shrink_opt = 0,
                  double tot_uni = -1.0, Label backoff_label = 0,
                  double norm_eps = kNormEps, bool check_consistency = false)
      : NGramShrink(infst, shrink_opt < 2 ? shrink_opt : 0, tot_uni,
                    backoff_label, norm_eps, check_consistency) {
    // shrink_opt must be less than 2 for count pruning
    for (int i = 0; i < HiOrder(); ++i) {  // initialize minimum values
      count_minimums_[i] = count_minimums.size() > i
                               ? count_minimums[i]
                               : StdArc::Weight::Zero().Value();
    }
  }

  ~NGramCountPrune() override {}

  // Shrinks n-gram model, based on initialized parameters
  bool ShrinkNGramModel() {
    return NGramShrink<StdArc>::ShrinkNGramModel(false);
  }

 protected:
  // Gives the pruning threshold (based on input count minimums)
  double GetTheta(StateId state) const override {
    return count_minimums_[StateOrder(state) - 1];
  }

 private:
  // Checks if character is digit or decimal
  bool IsInNumber(char c) const { return (c >= '0' && c <= '9') || c == '.'; }

  // Stores character and moves string iterator to next position
  char GetNextChar(string::const_iterator *strit) const {
    char c = (*(*strit));
    ++(*strit);
    return c;
  }

  // Reads from string while token is a numerical value
  template <class A>
  char GetNextCharVal(string::const_iterator *strit, A *toget,
                      const string &count_pattern);

  // Derives count minimums from input count pruning string.
  void ParseCountMinimums(const string &count_pattern);

  // Updates count minimums for order, based on parsed parameter string
  void UpdateCountMinimums(int order, double count, bool plus);

  std::vector<double> count_minimums_;  // minimums for count pruning
};

}  // namespace ngram

#endif  // NGRAM_NGRAM_COUNT_PRUNE_H_
