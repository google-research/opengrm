
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
// NGram model class for reading in a model or text for building a model.

#ifndef NGRAM_NGRAM_INPUT_H_
#define NGRAM_NGRAM_INPUT_H_

#include <fstream>
#include <istream>
#include <ostream>
#include <sstream>

#include <fst/arcsort.h>
#include <fst/fst.h>
#include <fst/matcher.h>
#include <fst/mutable-fst.h>
#include <fst/vector-fst.h>
#include <ngram/ngram-count.h>
#include <ngram/ngram-model.h>
#include <ngram/ngram-mutable-model.h>
#include <ngram/util.h>

namespace ngram {

using ngram::NGramMutableModel;
using fst::LogWeightTpl;
using fst::StdVectorFst;
using fst::SymbolTable;
using std::vector;

// Read vector of tokens from string.
void ReadTokenString(const string &str, vector<string> *words);

class NGramInput {
 public:
  typedef StdArc Arc;
  typedef Arc::StateId StateId;
  typedef Arc::Label Label;
  typedef Arc::Weight Weight;

  // Construct an NGramInput object, consisting of a symbol table, an FST,
  // and associated input and output streams.
  NGramInput(const string &ifile, const string &ofile, const string &symbols,
             const string &epsilon_symbol, const string &OOV_symbol,
             const string &start_symbol, const string &end_symbol)
      : oov_symbol_(OOV_symbol),
        start_symbol_(start_symbol),
        end_symbol_(end_symbol),
        error_(false) {
    InitializeIStream(ifile, nullptr);
    InitializeOStream(ofile, nullptr);
    InitializeSymbols(symbols, epsilon_symbol);
  }

  NGramInput(std::istream *istrm, std::ostream *ostrm, const string &symbols,
             const string &epsilon_symbol, const string &OOV_symbol,
             const string &start_symbol, const string &end_symbol)
      : oov_symbol_(OOV_symbol),
        start_symbol_(start_symbol),
        end_symbol_(end_symbol),
        error_(false) {
    InitializeIStream("", istrm);
    InitializeOStream("", ostrm);
    InitializeSymbols(symbols, epsilon_symbol);
  }

  // Read text input of three types: ngram counts, ARPA model or text corpus
  // output either model fsts, a corpus far or a symbol table.
  bool ReadInput(bool ARPA, bool symbols, bool output = true,
                 bool renormalize_arpa = false) {
    if (Error()) return false;
    if (ARPA) {  // ARPA format model in, fst model out
      return CompileARPAModel(output, renormalize_arpa);
    } else if (symbols) {
      return CompileSymbolTable(output);
    } else {  // sorted list of ngrams + counts in, fst out
      return CompileNGramCounts(output);
    }
    return false;
  }

  const MutableFst<Arc> *GetFst() const { return fst_.get(); }

  // Returns true if input setup is in a bad state.
  bool Error() const { return error_; }

 protected:
  void SetError() { error_ = true; }

 private:
  void InitializeSymbols(const string &symbols, const string &epsilon_symbol) {
    if (Error()) return;
    if (symbols == "") {  // Symbol table not provided
      syms_.reset(new SymbolTable("NGramSymbols"));  // initialize symbol table
      syms_->AddSymbol(epsilon_symbol);              // make epsilon 0
      add_symbols_ = 1;
    } else {
      syms_.reset(SymbolTable::ReadText(symbols));
      if (!syms_) {
        NGRAMERROR() << "NGramInput: Could not read symbol table file: "
                     << symbols;
        SetError();
        return;
      }
      add_symbols_ = 0;
    }
  }

  void InitializeIStream(const string &ifile, std::istream *in_stream) {
    if (Error()) return;
    if (ifile.empty() && in_stream == nullptr) {
      istrm_ = &std::cin;
    } else if (in_stream != nullptr) {
      istrm_ = in_stream;
    } else {
      ifstrm_.open(ifile);
      if (!ifstrm_) {
        LOG(ERROR) << "NGramInput: Can't open " << ifile << " for reading";
        SetError();
        return;
      }
      istrm_ = &ifstrm_;
    }
  }

  void InitializeOStream(const string &ofile, std::ostream *out_stream) {
    if (Error()) return;
    if (ofile.empty() && out_stream == nullptr) {
      ostrm_ = &std::cout;
    } else if (out_stream != nullptr) {
      ostrm_ = out_stream;
    } else {
      ofstrm_.open(ofile);
      if (!ofstrm_) {
        LOG(ERROR) << "NGramInput: Can't open " << ofile << " for writing";
        SetError();
        return;
      }
      ostrm_ = &ofstrm_;
    }
  }

  // Using whitespace as delimiter, reads token from string.
  bool GetWhiteSpaceToken(string::iterator *strit, string *str, string *token) {
    while (isspace(*(*strit)))  // skip the whitespace preceding the token
      (*strit)++;
    if ((*strit) == str->end())  // no further tokens to be found in string
      return false;
    while ((*strit) < str->end() && !isspace(*(*strit))) {
      (*token) += (*(*strit));
      (*strit)++;
    }
    return true;
  }

  // Gets symbol label from table, add it and ensure no duplicates if requested.
  Label GetLabel(const string &word, bool add, bool dups) {
    Label symlabel = syms_->Find(word);  // find it in the symbol table
    if (!add_symbols_) {                 // fixed symbol table provided
      if (symlabel == fst::kNoLabel) {
        symlabel = syms_->Find(oov_symbol_);
        if (symlabel == fst::kNoLabel) {
          NGRAMERROR() << "NGramInput: OOV Symbol not found "
                       << "in given symbol table: " << oov_symbol_;
          SetError();
        }
      }
    } else if (add) {
      if (symlabel == fst::kNoLabel) {
        symlabel = syms_->AddSymbol(word);
      } else if (!dups) {  // shouldn't find duplicate
        NGRAMERROR() << "NGramInput: Symbol already found in list: " << word;
        SetError();
      }
    } else if (symlabel== fst::kNoLabel) {
      NGRAMERROR() << "NGramInput: Symbol not found in list: " << word;
      SetError();
    }
    return symlabel;
  }

  // GetLabel() if not <s> or </s>, otherwise set appropriate bool values
  Label GetNGramLabel(const string &ngram_word, bool add, bool dups,
                      bool *stsym, bool *endsym) {
    (*stsym) = (*endsym) = 0;
    if (ngram_word == start_symbol_) {
      (*stsym) = 1;
      return -1;
    } else if (ngram_word == end_symbol_) {
      (*endsym) = 1;
      return -2;
    } else {
      return GetLabel(ngram_word, add, dups);
    }
  }

  // Uses string iterator to construct token, then gets the label for it.
  Label ExtractNGramLabel(string::iterator *strit, string *str, bool add,
                          bool dups, bool *stsym, bool *endsym) {
    string token;
    if (!GetWhiteSpaceToken(strit, str, &token)) {
      NGRAMERROR() << "NGramInput: No token found when expected";
      SetError();
      return -1;
    }
    return GetNGramLabel(token, add, dups, stsym, endsym);
  }

  // Gets backoff state and backoff cost for state (following <epsilon> arc).
  StateId GetBackoffAndCost(StateId st, double *cost) {
    StateId backoff = -1;
    Label backoff_label = 0;  // <epsilon> is assumed to be label 0 here
    Matcher<StdFst> matcher(*fst_, MATCH_INPUT);
    matcher.SetState(st);
    if (matcher.Find(backoff_label)) {
      for (; !matcher.Done(); matcher.Next()) {
        const StdArc &arc = matcher.Value();
        if (arc.ilabel == backoff_label) {
          backoff = arc.nextstate;
          if (cost) (*cost) = arc.weight.Value();
        }
      }
    }
    return backoff;
  }

  // Just returns backoff state.
  StateId GetBackoff(StateId st) { return GetBackoffAndCost(st, nullptr); }

  // Ensure matching with appropriate ARPA header strings
  bool ARPAHeaderStringMatch(const string &tomatch) {
    string str;
    if (!getline((*istrm_), str)) {
      NGRAMERROR() << "Input stream read error";
      SetError();
      return false;
    }
    if (str != tomatch) {
      str += "   Line should read: ";
      str += tomatch;
      NGRAMERROR() << "NGramInput: ARPA header mismatch!  Line reads: " << str;
      SetError();
      return false;
    }
    return true;
  }

  // Extract string token and convert into value of type A
  template <class A>
  bool GetStringVal(string::iterator *strit, string *str, A *val,
                    string *keeptoken) {
    string token;
    if (GetWhiteSpaceToken(strit, str, &token)) {
      std::stringstream ngram_ss(token);
      ngram_ss >> (*val);
      if (keeptoken) (*keeptoken) = token;  // to store token string if needed
      return 1;
    } else {
      return 0;
    }
  }

  // When reading in string numerical tokens, ensures correct inf values
  void CheckInfVal(string *token, double *val) {
    if ((*token) == "-inf" || (*token) == "-Infinity") (*val) = log(0);
    if ((*token) == "inf" || (*token) == "Infinity") (*val) = -log(0);
  }

  // Read the header at the top of the ARPA model file, collect n-gram orders
  int ReadARPATopHeader(vector<int> *orders) {
    string str;
    // scan the file until a \data\ record is found
    while (getline((*istrm_), str)) {
      if (str == "\\data\\") break;
    }
    if (!getline((*istrm_), str)) {
      NGRAMERROR() << "Input stream read error, or no \\data\\ record found";
      SetError();
      return 0;
    }
    int order = 0;
    while (str != "") {
      string::iterator strit = str.begin();
      while (strit < str.end() && (*strit) != '=') strit++;
      if (strit == str.end()) {
        NGRAMERROR()
            << "NGramInput: ARPA header mismatch!  No '=' in ngram count.";
        SetError();
        return 0;
      }
      strit++;
      int ngram_cnt;  // must have n-gram count, fails if not found
      if (!GetStringVal(&strit, &str, &ngram_cnt, nullptr)) {
        NGRAMERROR() << "NGramInput: ARPA header mismatch!  No ngram count.";
        SetError();
        return 0;
      }
      orders->push_back(ngram_cnt);
      if (ngram_cnt > 0) order++;  // Some reported n-gram orders may be empty
      if (!getline((*istrm_), str)) {
        NGRAMERROR() << "Input stream read error";
        SetError();
        return 0;
      }
    }
    return order;
  }

  // Get the destination state of arc with requested label.  Assumed to exist.
  StateId GetLabelNextState(StateId st, Label label) {
    Matcher<StdFst> matcher(*fst_, MATCH_INPUT);
    matcher.SetState(st);
    if (matcher.Find(label)) {
      StdArc barc = matcher.Value();
      return barc.nextstate;
    } else {
      NGRAMERROR() << "NGramInput: Lower order prefix n-gram not found: ";
      SetError();
      return -1;
    }
  }

  // Get the destination state of arc with requested label.  Assumed to exist.
  StateId GetLabelNextStateNoFail(StateId st, Label label) {
    Matcher<StdFst> matcher(*fst_, MATCH_INPUT);
    matcher.SetState(st);
    if (matcher.Find(label)) {
      StdArc barc = matcher.Value();
      return barc.nextstate;
    } else {
      return -1;
    }
  }

  // GetLabelNextState() when arc exists; other results for <s> and </s>
  ssize_t NextStateFromLabel(
      ssize_t st, Label label, bool stsym, bool endsym,
      NGramCounter<LogWeightTpl<double>> *ngram_counter) {
    if (Error()) return 0;
    if (stsym) {  // start symbol: <s>
      return ngram_counter->NGramStartState();
    } else if (endsym) {  // end symbol </s>
      NGRAMERROR() << "NGramInput: stop symbol occurred in n-gram prefix";
      SetError();
      return 0;
    } else {
      ssize_t arc_id = ngram_counter->FindArc(st, label);
      return ngram_counter->NGramNextState(arc_id);
    }
  }

  // Extract the token, find the label and the appropriate destination state
  ssize_t GetNextState(string::iterator *strit, string *str, ssize_t st,
                       NGramCounter<LogWeightTpl<double> > *ngram_counter) {
    if (Error()) return 0;
    bool stsym = false;
    bool endsym = false;
    auto label = ExtractNGramLabel(strit, str, /* add = */ false,
                                   /* dups = */ false, &stsym, &endsym);
    return NextStateFromLabel(st, label, stsym, endsym, ngram_counter);
  }

  // Read the header for each of the n-gram orders in the ARPA format file
  void ReadARPAOrderHeader(int order) {
    std::stringstream ss;
    ss << order + 1;
    string tomatch = "\\";
    tomatch += ss.str();
    tomatch += "-grams:";
    ARPAHeaderStringMatch(tomatch);
  }

  // Add an n-gram arc as appropriate and record the state and label if req'd
  void AddNGramArc(StateId st, StateId nextstate, Label label, bool stsym,
                   bool endsym, double ngram_log_prob) {
    if (endsym)  // </s> requires no arc, just final cost
      fst_->SetFinal(st, ngram_log_prob);
    else if (!stsym)  // create arc from st to nextstate
      fst_->AddArc(st, StdArc(label, label, ngram_log_prob, nextstate));
  }

  // Read in n-grams for the particular order.
  void ReadARPAOrder(vector<int> *orders, int order, vector<double> *boweights,
                     NGramCounter<LogWeightTpl<double>> *ngram_counter) {
    string str;
    bool add_words = (order == 0);
    for (auto i = 0; i < (*orders)[order]; i++) {
      if (!getline((*istrm_), str)) {
        NGRAMERROR() << "Input stream read error";
        SetError();
        return;
      }
      string::iterator strit = str.begin();
      double nlprob;
      double boprob;
      string token;
      if (!GetStringVal(&strit, &str, &nlprob, &token)) {
        NGRAMERROR() << "NGramInput: ARPA format mismatch!  No ngram log prob.";
        SetError();
        return;
      }
      CheckInfVal(&token, &boprob);  // check for inf value
      nlprob *= -log(10);  // convert to neglog base e from log base 10
      ssize_t st = ngram_counter->NGramUnigramState();
      StateId nextstate = fst::kNoStateId;
      for (auto j = 0; j < order; j++)  // find n-gram history state
        st = GetNextState(&strit, &str, st, ngram_counter);
      if (Error()) return;
      bool stsym;   // stsym == 1 for <s>.
      bool endsym;  // endsym == 1 for </s>.
      Label label = ExtractNGramLabel(&strit, &str, add_words,
                                      /* dupls = */ false, &stsym, &endsym);
      if (Error()) return;
      if (endsym) {
        ngram_counter->SetFinalNGramWeight(st, nlprob);
      } else if (!stsym) {
        // Test for presence of all suffixes of n-gram
        auto backoff_st = ngram_counter->NGramBackoffState(st);
        while (backoff_st >= 0) {
          ngram_counter->FindArc(backoff_st, label);
          backoff_st = ngram_counter->NGramBackoffState(backoff_st);
        }
        auto arc_id = ngram_counter->FindArc(st, label);
        ngram_counter->SetNGramWeight(arc_id, nlprob);
        nextstate = ngram_counter->NGramNextState(arc_id);
      } else {
        nextstate = ngram_counter->NGramStartState();
      }
      if (GetStringVal(&strit, &str, &boprob, &token) &&
          (nextstate >= 0 || boprob != 0)) {  // found non-zero backoff cost
        if (nextstate == fst::kNoStateId) {
          NGRAMERROR() << "NGramInput: Have a backoff cost with no state ID!";
          SetError();
          return;
        }
        CheckInfVal(&token, &boprob);  // check for inf value
        boprob *= -log(10);  // convert to neglog base e from log base 10
        while (nextstate >= boweights->size())
          boweights->push_back(StdArc::Weight::Zero().Value());
        (*boweights)[nextstate] = boprob;
      }
    }
    // blank line at end of n-gram order
    if (!getline((*istrm_), str)) {
      NGRAMERROR() << "Input stream read error";
      SetError();
      return;
    }
    if (!str.empty()) {
      NGRAMERROR() << "Expected blank line at end of n-grams";
      SetError();
    }
  }

  StateId FindNewDest(StateId st) {
    StateId newdest = st;
    if (fst_->NumArcs(st) > 1 || fst_->Final(st) != StdArc::Weight::Zero())
      return newdest;
    MutableArcIterator<StdMutableFst> aiter(fst_.get(), st);
    const StdArc &arc = aiter.Value();
    if (arc.ilabel == 0) newdest = FindNewDest(arc.nextstate);
    return newdest;
  }

  void SetARPANGramDests() {
    vector<StateId> newdests;
    newdests.reserve(fst_->NumStates());
    for (StateId st = 0; st < fst_->NumStates(); ++st)
      newdests.push_back(FindNewDest(st));
    for (auto st = 0; st < fst_->NumStates(); ++st) {
      for (MutableArcIterator<StdMutableFst> aiter(fst_.get(), st);
           !aiter.Done(); aiter.Next()) {
        StdArc arc = aiter.Value();
        if (arc.ilabel == 0) continue;
        if (newdests[arc.nextstate] != arc.nextstate) {
          arc.nextstate = newdests[arc.nextstate];
          aiter.SetValue(arc);
        }
      }
    }
  }

  // Put stored backoff weights on backoff arcs
  void SetARPABackoffWeights(vector<double> *boweights) {
    for (auto st = 0; st < fst_->NumStates(); ++st) {
      if (st < boweights->size()) {
        double boprob = (*boweights)[st];
        MutableArcIterator<StdMutableFst> aiter(fst_.get(), st);
        StdArc arc = aiter.Value();
        if (arc.ilabel == 0 || boprob != StdArc::Weight::Zero().Value()) {
          if (arc.ilabel != 0) {
            NGRAMERROR() << "NGramInput: Have a backoff prob but no arc";
            SetError();
            return;
          } else {
            arc.weight = boprob;
          }
          aiter.SetValue(arc);
        }
      }
    }
  }

  double GetLowerOrderProb(StateId st, Label label) {
    Matcher<StdFst> matcher(*fst_, MATCH_INPUT);
    matcher.SetState(st);
    if (matcher.Find(label)) {
      const StdArc &arc = matcher.Value();
      return arc.weight.Value();
    }
    if (!matcher.Find(0)) {
      NGRAMERROR() << "NGramInput: No backoff probability";
      SetError();
      return StdArc::Weight::Zero().Value();
    }
    for (; !matcher.Done(); matcher.Next()) {
      const StdArc &arc = matcher.Value();
      if (arc.ilabel == 0) {
        return arc.weight.Value() + GetLowerOrderProb(arc.nextstate, label);
      }
    }
    NGRAMERROR() << "NGramInput: No backoff arc found";
    SetError();
    return StdArc::Weight::Zero().Value();
  }

  // Descends backoff arcs to find backoff final cost and set
  double GetFinalBackoff(StateId st) {
    if (fst_->Final(st) != StdArc::Weight::Zero())
      return fst_->Final(st).Value();
    double bocost;
    auto bostate = GetBackoffAndCost(st, &bocost);
    if (bostate >= 0) fst_->SetFinal(st, bocost + GetFinalBackoff(bostate));
    return fst_->Final(st).Value();
  }

  void FillARPAHoles() {
    for (auto st = 0; st < fst_->NumStates(); ++st) {
      double boprob;
      StateId bostate = -1;
      for (MutableArcIterator<StdMutableFst> aiter(fst_.get(), st);
           !aiter.Done(); aiter.Next()) {
        auto arc = aiter.Value();
        if (arc.ilabel == 0) {
          boprob = arc.weight.Value();
          bostate = arc.nextstate;
        } else {
          if (arc.weight == StdArc::Weight::Zero()) {
            arc.weight = boprob + GetLowerOrderProb(bostate, arc.ilabel);
            if (Error()) return;
            aiter.SetValue(arc);
          }
        }
      }
      if (bostate >= 0 && fst_->Final(st) != StdArc::Weight::Zero() &&
          fst_->Final(bostate) == StdArc::Weight::Zero()) {
        GetFinalBackoff(bostate);
      }
    }
  }

  // Read in headers/n-grams from an ARPA model text file, dump resulting fst
  bool CompileARPAModel(bool output, bool renormalize) {
    vector<int> orders;
    ReadARPATopHeader(&orders);
    if (Error()) return false;
    vector<double> boweights;
    NGramCounter<LogWeightTpl<double> > ngram_counter(orders.size());
    for (auto i = 0; i < orders.size(); i++) {  // Read n-grams of each order
      ReadARPAOrderHeader(i);
      if (Error()) return false;
      ReadARPAOrder(&orders, i, &boweights, &ngram_counter);
      if (Error()) return false;
    }
    ARPAHeaderStringMatch("\\end\\");  // Verify that everything parsed well
    if (Error()) return false;
    fst_.reset(new StdVectorFst());
    ngram_counter.GetFst(fst_.get());
    ArcSort(fst_.get(), StdILabelCompare());
    SetARPABackoffWeights(&boweights);
    if (Error()) return false;
    FillARPAHoles();
    if (Error()) return false;
    SetARPANGramDests();
    Connect(fst_.get());
    if (renormalize) RenormalizeARPAModel();
    if (Error()) return false;
    DumpFst(true, output);
    return true;
  }

  // Renormalizes the ARPA format model if required.
  void RenormalizeARPAModel() {
    NGramMutableModel<Arc> ngram_model(fst_.get());
    if (ngram_model.CheckNormalization() || ngram_model.Error()) return;
    StateId st = ngram_model.UnigramState();
    if (st == fst::kNoStateId) st = fst_->Start();
    double renorm_val = ngram_model.ScalarValue(fst_->Final(st));
    double KahanVal = 0.0;
    for (ArcIterator<MutableFst<Arc>> aiter(*fst_, st); !aiter.Done();
         aiter.Next()) {
      Arc arc = aiter.Value();
      renorm_val =
          NegLogSum(renorm_val, ngram_model.ScalarValue(arc.weight), &KahanVal);
    }
    if (fst_->Final(st) != Arc::Weight::Zero()) {
      fst_->SetFinal(st, ngram_model.ScaleWeight(fst_->Final(st), -renorm_val));
    }
    for (MutableArcIterator<MutableFst<Arc>> aiter(fst_.get(), st);
         !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
      arc.weight = ngram_model.ScaleWeight(arc.weight, -renorm_val);
      aiter.SetValue(arc);
    }
    ngram_model.RecalcBackoff();
    if (!ngram_model.CheckNormalization()) {
      NGRAMERROR() << "ARPA model could not be renormalized";
      SetError();
    }
  }

  // Redirects hi order arcs in acyclic count format to proper next states.
  void MakeCyclicTopology(StateId st, StateId bo,
                          const vector<bool> &bo_incoming) {
    for (MutableArcIterator<StdMutableFst> aiter(fst_.get(), st); !aiter.Done();
         aiter.Next()) {
      auto arc = aiter.Value();
      auto nst = GetLabelNextState(bo, arc.ilabel);
      if (!bo_incoming[arc.nextstate] &&
          fst_->Final(arc.nextstate) == StdArc::Weight::Zero() &&
          fst_->NumArcs(arc.nextstate) == 0) {  // if nextstate not in model
        arc.nextstate = nst;  // point to state that will persist in the model
        aiter.SetValue(arc);
      } else {
        MakeCyclicTopology(arc.nextstate, nst, bo_incoming);
      }
    }
  }

  // Collects state level information prior to changing topology.
  void SetStateBackoff(StateId st, StateId bo, vector<StateId> *bo_dest,
                       vector<double> *total_cnt, vector<bool> *bo_incoming) {
    (*bo_dest)[st] = bo;  // Records the backoff state to be added later.

    // Records that state is backed off to by another state in the model.
    (*bo_incoming)[bo] = true;
    (*total_cnt)[st] = fst_->Final(st).Value();
    double correction_value = 0.0;
    for (MutableArcIterator<StdMutableFst> aiter(fst_.get(), st); !aiter.Done();
         aiter.Next()) {
      auto arc = aiter.Value();
      (*total_cnt)[st] = NegLogSum((*total_cnt)[st],
                                   arc.weight.Value(), &correction_value);
      auto nst = GetLabelNextState(bo, arc.ilabel);
      SetStateBackoff(arc.nextstate, nst, bo_dest, total_cnt, bo_incoming);
    }
  }

  // Create re-entrant model topology from acyclic count automaton
  void AddBackoffAndCycles(StateId Unigram, Label bo_label) {
    ArcSort(fst_.get(), StdILabelCompare());  // Ensures arcs fully sorted
    vector<StateId> bo_dest(fst_->NumStates(), -1);
    vector<bool> bo_incoming(fst_->NumStates(), false);
    vector<double> total_cnt(fst_->NumStates(), StdArc::Weight::Zero().Value());

    // Stores all bigram states in a vector for ascending state functions.
    std::vector<StateId> bigram_states;
    if (fst_->Start() != Unigram) bigram_states.push_back(fst_->Start());
    for (ArcIterator<StdMutableFst> aiter(*fst_, Unigram); !aiter.Done();
         aiter.Next()) {
      const StdArc &arc = aiter.Value();
      bigram_states.push_back(arc.nextstate);
    }

    // Ascends to all states and collects state information.
    for (auto i = 0; i < bigram_states.size(); ++i) {
      SetStateBackoff(bigram_states[i], Unigram, &bo_dest, &total_cnt,
                      &bo_incoming);
    }

    // Ascends to all states from unigram and makes topology cyclic.
    for (auto i = 0; i < bigram_states.size(); ++i)
      MakeCyclicTopology(bigram_states[i], Unigram, bo_incoming);

    // Adds backoff arcs for each state in the topology.
    for (auto st = 0; st < fst_->NumStates(); ++st) {  // add backoff arcs
      if (bo_dest[st] >= 0)  // if backoff state has been recorded
        fst_->AddArc(st,
                     StdArc(bo_label, bo_label, total_cnt[st], bo_dest[st]));
    }
    ArcSort(fst_.get(), StdILabelCompare());  // resorts for new backoff arcs
    Connect(fst_.get());  // connects to dispose of states not in the model
  }

  // Control allocation of ARPA model start state; evidence comes incrementally
  void CheckInitState(vector<string> *words, StateId *Init, StateId Unigram,
                      StateId Start) {
    if (words->size() > 2 && (*Init) == Unigram) {  // 1st evidence order > 1
      if (Start >= 0)  // if start unigram already seen, use state as initial
        (*Init) = Start;
      else  // otherwise, need to create a start state
        (*Init) = fst_->AddState();
      fst_->SetStart((*Init));  // Set it as start state
    }
  }

  // Reads in N-gram tokens as well as count (last token) from string.
  double ReadNGramFromString(string str, vector<string> *words, StateId *Init,
                             StateId Unigram, StateId Start) {
    ReadTokenString(str, words);
    if (words->empty()) {
      NGRAMERROR() << "NGramInput: empty line in file: format error";
      SetError();
      return 0.0;
    }
    std::stringstream cnt_ss(
        (*words)[words->size() - 1]);  // The last token is the count.
    double ngram_count;
    cnt_ss >> ngram_count;
    CheckInitState(words, Init, Unigram, Start);  // Checks start state status.
    return -log(ngram_count);  // Counts are encoded in -log-space.
  }

  // Iterates through words in the n-gram history to find current state.
  StateId GetHistoryState(vector<string> *words, vector<Label> *last_labels,
                          vector<StateId> *last_states, StateId st) {
    for (auto i = 0; i < words->size() - 2; i++) {
      bool stsym;
      bool endsym;
      Label label = GetNGramLabel((*words)[i], false, false, &stsym, &endsym);
      if (Error()) return 0;
      if (last_labels->size() <= i || label != (*last_labels)[i]) {
        string prefix = (*words)[0];
        for (auto j = 1; j <= i; ++j) prefix += " " + (*words)[j];
        NGRAMERROR() << "NGramInput: n-gram prefix (" << prefix
                     << ") not seen in previous n-gram";
        SetError();
        return 0;
      }
      st = (*last_states)[i];  // Retrieves previously stored state.
    }
    return st;
  }

  // When reading in final token of n-gram, determines the nextstate.
  StateId GetCntNextSt(StateId st, StateId Unigram, StateId Init,
                       StateId *Start, bool stsym, bool endsym) {
    StateId nextstate = -1;
    if (stsym) {            // Start symbol: <s>
      if (st != Unigram) {  // Should not occur.
        NGRAMERROR() << "NGramInput: start symbol occurred in n-gram suffix";
        SetError();
        return nextstate;
      }
      if (Init == Unigram)            // Don't know if model is order > 1 yet
        (*Start) = fst_->AddState();  // Create state associated with <s>
      else  // Already created 2nd order Start state, stored as Init
        (*Start) = Init;
      nextstate = (*Start);
    } else if (!endsym) {  // not a </s> symbol, hence need to create a state
      nextstate = fst_->AddState();
    }
    return nextstate;
  }

  // Updates last label and state, for retrieval with following n-grams.
  int UpdateLast(vector<string> *words, int longest_ngram,
                 vector<Label> *last_labels, vector<StateId> *last_states,
                 Label label, StateId nextst) {
    if (words->size() > longest_ngram + 1) {  // Adds a dimension to vectors.
      longest_ngram++;
      last_labels->push_back(-1);
      last_states->push_back(-1);
    }
    (*last_labels)[words->size() - 2] = label;
    (*last_states)[words->size() - 2] = nextst;
    return longest_ngram;
  }

  // Reads in a sorted n-gram count file, converting to OpenGrm format.
  bool CompileNGramCounts(bool output) {
    fst_.reset(new StdVectorFst());  // Creates new FST.
    auto init = fst_->AddState();
    auto unigram = init;
    auto start = fst::kNoStateId;
    int longram = 0;  // Keeps track of longest observed n-gram in file.
    string str;
    vector<Label> last_labels;         // Stores labels from prior n-grams.
    vector<StateId> last_states;       // Stores states from prior n-grams.
    while (getline((*istrm_), str)) {  // For each string...
      vector<string> words;
      // Reads in word tokens from string, and returns count.
      double ngram_count =  ReadNGramFromString(str, &words, &init, unigram,
                                                start);
      if (Error()) return false;
      // Finds n-gram history state from prefix words in n-gram.
      auto st = GetHistoryState(&words, &last_labels, &last_states, unigram);
      if (Error()) return false;
      bool stsym;
      bool endsym;
      // Gets label of word suffix of n-gram.
      auto label =  GetNGramLabel(words[words.size() - 2], 1, 1, &stsym,
                                  &endsym);
      if (Error()) return false;
      // Gets the next state from history state and label.
      auto nextst = GetCntNextSt(st, unigram, init, &start, stsym, endsym);
      if (Error()) return false;
      // Adds arc.
      AddNGramArc(st, nextst, label, stsym, endsym, ngram_count);
      // Updates states and labels for subsequent n-grams.
      longram = UpdateLast(&words, longram, &last_labels, &last_states, label,
                           nextst);
    }
    if (init == unigram)  // Sets init as start state for unigram model.
      fst_->SetStart(init);
    AddBackoffAndCycles(unigram, 0);  // Turns into reentrant OpenGrm format.
    DumpFst(true, output);
    return true;
  }

  // Tokenizes string and store labels in a vector for building an FST.
  double FillStringLabels(string *str, vector<Label> *labels,
                          bool string_counts) {
    string token = "";
    auto strit = str->begin();
    double count = 1.0;
    if (string_counts) {
      GetWhiteSpaceToken(&strit, str, &token);
      count = atof(token.c_str());
      token = "";
    }
    while (GetWhiteSpaceToken(&strit, str, &token)) {
      labels->push_back(GetLabel(token, true, true));  // Stores index.
      token = "";
    }
    return count;
  }

  // From text corpus to symbol table
  bool CompileSymbolTable(bool output) {
    string str;
    bool gotline = static_cast<bool>(getline((*istrm_), str));
    while (gotline) {  // for each string
      vector<Label> labels;
      FillStringLabels(&str, &labels, false);
      if (Error()) return false;
      gotline = static_cast<bool>(getline((*istrm_), str));
    }
    if (!oov_symbol_.empty()) syms_->AddSymbol(oov_symbol_);
    if (output) syms_->WriteText(*ostrm_);
    return true;
  }

  // Write resulting fst to specified stream
  void DumpFst(bool incl_symbols, bool output) {
    if (incl_symbols) {
      fst_->SetInputSymbols(syms_.get());
      fst_->SetOutputSymbols(syms_.get());
    }
    if (output) fst_->Write(*ostrm_, fst::FstWriteOptions());
  }

  std::unique_ptr<MutableFst<Arc>> fst_;
  std::unique_ptr<SymbolTable> syms_;
  bool add_symbols_;
  string oov_symbol_;
  string start_symbol_;
  string end_symbol_;
  std::ifstream ifstrm_;
  std::ofstream ofstrm_;
  std::istream *istrm_;
  std::ostream *ostrm_;
  bool error_;
};

}  // namespace ngram

#endif  // NGRAM_NGRAM_INPUT_H_
