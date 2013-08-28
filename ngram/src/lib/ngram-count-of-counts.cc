// ngram-count-of-counts.cc
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
// Class for computing/accessing count-of-count bins for
// e.g., Katz and absolute discounting.

#include<sstream>

#include<ngram/ngram-count-of-counts.h>

namespace ngram {

// Calculate count histograms
void NGramCountOfCounts::CalculateCounts(const NGramModel &model) {
  if (!histogram_.empty())
    return;
  histogram_.resize(model.HiOrder());
  for (int order = 0; order < model.HiOrder(); ++order)  // for each order
    histogram_[order].resize(bins_ + 1, 0.0);  // space for bins + 1

  for (StateId st = 0; st < model.NumStates(); ++st) {  // get histograams
    if (!context_.NullContext()) {  // restricted context
      const vector<Label> &ngram = model.StateNGram(st);
      if (!context_.HasContext(ngram, false))
        continue;
    }
    int order = model.StateOrder(st) - 1;  // order starts from 0 here, not 1
    for (ArcIterator<StdFst> aiter(model.GetFst(), st);
         !aiter.Done();
         aiter.Next()) {
      StdArc arc = aiter.Value();
      if (arc.ilabel != model.BackoffLabel())  // no count from backoff
        IncrementBinCount(order, arc.weight.Value());
    }
    IncrementBinCount(order, model.GetFst().Final(st).Value());
  }
}

// Display input histogram
void NGramCountOfCounts::ShowCounts(const vector < vector <double> > &hist,
                                    const string &label) const {
  int hi_order = hist.size();
  cerr << "Count bin   ";
  cerr << label;
  cerr << " Counts (";
  for (int order = 0; order < hi_order; ++order) {
    if (order > 0) cerr << "/";
    cerr << order + 1 << "-grams";
  }
  cerr << ")\n";
  for (int bin = 0; bin <= bins_; ++bin) {
    if (bin < bins_)
      cerr << "Count = " << bin + 1 << "   ";
    else
      cerr << "Count > " << bin  << "   ";
    for (int order = 0; order < hi_order; ++order) {
      if (order > 0) cerr << "/";
      cerr << hist[order][bin];
    }
    cerr << "\n";
  }
}

// Get an Fst representation of the ngram count-of-counts
void NGramCountOfCounts::GetFst(StdMutableFst *fst) const {
  SymbolTable *symbols = new SymbolTable();

  fst->DeleteStates();
  StateId s = fst->AddState();
  fst->SetStart(s);
  int hi_order = histogram_.size();
  symbols->AddSymbol("<epsilon>", 0);
  double sum = kFloatEps;
  for (int order = 0; order < hi_order; ++order) {
    for (int bin = 0; bin <= bins_; ++bin) {
      // label encodes order and bin
      Label label = order * (kMaxBins + 1) + bin + 1;
      ostringstream strm;
      strm << "order=" << order << ",bin=" << bin;
      symbols->AddSymbol(strm.str(), label);
      Weight weight = -log(histogram_[order][bin]);
      if (bin > 0 && weight == Weight::Zero())
        continue;
      fst->AddArc(s, StdArc(label, label, weight, s));
      sum += histogram_[order][bin];
    }
  }
  fst->SetFinal(s, -log(sum));
  fst->SetInputSymbols(symbols);
  fst->SetOutputSymbols(symbols);
  delete symbols;
}

// Sets counts from count-of-counts FST
void NGramCountOfCounts::SetCounts(const StdFst &fst) {
  histogram_.clear();
  if (fst.Start() == kNoStateId)
    return;

  for (ArcIterator<StdFst> aiter(fst, 0); !aiter.Done(); aiter.Next()) {
    StdArc arc = aiter.Value();
    // label encodes order and bin
    int bin = (arc.ilabel - 1) % (kMaxBins + 1);
    int order = (arc.ilabel - 1) / (kMaxBins + 1);
    while (order >= histogram_.size())
      histogram_.push_back(vector<double>(bins_ + 1, 0.0));
    if (bin <= bins_)
      histogram_[order][bin] = round(exp(-arc.weight.Value()));
  }
}

const int NGramCountOfCounts::kMaxBins = 32;

}  // namespace ngram
