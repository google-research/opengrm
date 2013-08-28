// ngramrandtest.cc
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
// Generates random sentences from an LM or more generally paths through any
// FST where epsilons are treated as failure transitions.

#include <string>
#include <fst/fst.h>
#include <fst/vector-fst.h>
#include <fst/shortest-path.h>
#include <fst/rmepsilon.h>
#include <fst/randgen.h>
#include <fst/extensions/far/far.h>

#include <ngram/ngram-randgen.h>
#include <ngram/ngram-count.h>
#include <ngram/ngram-make.h>
#include <ngram/ngram-kneser-ney.h>
#include <ngram/ngram-absolute.h>
#include <ngram/ngram-katz.h>
#include <ngram/ngram-witten-bell.h>
#include <ngram/ngram-count-merge.h>
#include <ngram/ngram-context-merge.h>
#include <ngram/ngram-model-merge.h>
#include <ngram/ngram-output.h>
#include <ngram/ngram-shrink.h>
#include <ngram/ngram-seymore-shrink.h>

using namespace fst;
using namespace ngram;

typedef StdArc::StateId StateId;

DEFINE_int32(max_length, 1000, "Maximum sentence length");
DEFINE_int32(seed, time(0) + getpid(), "Randomization seed");
DEFINE_int32(vocabulary_max, 5000, "maximum vocabulary size");
DEFINE_int32(mean_length, 100, "maximum mean string length");
DEFINE_int32(sample_max, 10000, "maximum sample corpus size");
DEFINE_int32(ngram_max, 3, "maximum n-gram order size");
DEFINE_string(directory, ".", "directory where files will be placed");
DEFINE_string(vars, "", "file name for outputting variable values");
DEFINE_double(thresh_max, 3, "maximum threshold size");

// Calculate - log( exp(a - b) + 1 ) for use in high precision NegLogSum
static double NegLogDeltaValue(double a, double b, double *c) {
  double x = exp(a - b), delta = - log(x + 1);
  if (x < kNormEps) {  // for small x, use Mercator Series to calculate
    delta = -x;
    for (int j = 2; j <= 4; ++j)
      delta += pow(-x, j) / j;
  }
  if (c) delta -= (*c);  // Sum correction from Kahan formula (if using)
  return delta;
}

// Precision method for summing reals and saving negative logs
// -log( exp(-a) + exp(-b) ) = a - log( exp(a - b) + 1 )
// Uses Mercator series and Kahan formula for additional numerical stability
static double NegLogSum(double a, double b, double *c) {
  if (a == StdArc::Weight::Zero().Value()) return b;
  if (b == StdArc::Weight::Zero().Value()) return a;
  if (a > b) return NegLogSum(b, a, c);
  double delta = NegLogDeltaValue(a, b, c), val = a + delta;
  if (c) (*c) = (val - a) - delta;  // update sum correction for Kahan formula
  return val;
}

// Builds random context splits over given interval
int BuildContexts(int start, int end, int max, ostream *cntxstrm) {
  int split = floor(max * (rand()/(RAND_MAX + 1.0)));
  while (split <= 0)
    split = floor(max * (rand()/(RAND_MAX + 1.0)));
  if (split <= start || split > end) {
    (*cntxstrm) << start << " : " << end << endl;  
  } else {
    BuildContexts(start, split, max, cntxstrm);
    BuildContexts(split, end, max, cntxstrm);
  }
  return split;
}

// Builds a random unigram model based on a maximum vocabulary size and
// a maximum mean length of strings
void BuildRandomUnigram(StdMutableFst *unigram, int vocabulary_max,
			int mean_length, ostream *varstrm, ostream *cntxstrm) {
  int vocabulary = ceil(vocabulary_max * (rand()/(RAND_MAX + 1.0)));
  double mean_sent_length = mean_length * (rand()/(RAND_MAX + 1.0));
  BuildContexts(0, vocabulary + 1, vocabulary + 1, cntxstrm);
  SymbolTable syms;  // dummy symbol table
  syms.AddSymbol("0");  // add to dummy symbol table
  unigram->SetStart(unigram->AddState());  // single state automaton
  double C = 0, counts = -log(C);
  vector<double> weights;
  for (int i = 0; i < vocabulary; ++i) {  // for each word in vocabulary
    ostringstream idxlabel;
    idxlabel << i + 1;
    syms.AddSymbol(idxlabel.str());  // add to dummy symbol table
    weights.push_back(-log(rand() + 1));  // random -log count for unigram
    counts = NegLogSum(counts, weights[i], &C);  // for normalization
  }
  double final_cost = counts + log(mean_sent_length);  // how often </s> occurs
  counts = NegLogSum(counts, final_cost, &C);
  vector<double> wts;
  unigram->SetFinal(0, final_cost - counts);  // final cost
  wts.push_back(final_cost - counts);
  for (size_t a = 0; a < vocabulary; ++a)  {// add unigram arcs to model
    unigram->AddArc(0, StdArc(a+1, a+1, weights[a] - counts, 0));
    wts.push_back(weights[a] - counts);
  }
  unigram->SetInputSymbols(&syms);
  unigram->SetOutputSymbols(&syms);
}

// Sets up filenames for dumping randomly generated counts and models
string directory_label(int32 seed, string dir) {
  ostringstream seedlabel;
  seedlabel << seed;
  string directory = (dir == "") ? "" : dir + "/";
  directory += seedlabel.str() + ".";
  return directory;
}

// Sets up filenames for shard far files
string far_name(int32 far_num) {
  ostringstream far_label;
  far_label << far_num;
  string far_num_name =  "tocount.far." + far_label.str();
  return far_num_name;
}

// Adds Fst to FAR archive file
inline void AddToFar(MutableFst<StdArc>* stringfst,
		     int key_size,
		     int stringkey,
		     FarWriter<StdArc>* far_writer) {
  ostringstream keybuf;
  keybuf.width(key_size);
  keybuf.fill('0');
  keybuf << stringkey;
  string key;
  far_writer->Add(keybuf.str(), *stringfst);
}

// using an input model, generate a random corpus and count n-grams
int CountFromRandGen(StdMutableFst *genmodel, StdMutableFst *countfst,
		     NGramArcSelector<StdArc> *selector, int num_strings,
		     FarWriter<StdArc>* far_writer0,
		     int in_far_num, int max_length, int ngram_max,
		     string directory, bool first, ostream *varstrm) {
  int key_size = ceil(log10(2 * num_strings)) + 1, far_num = in_far_num,
    order = ceil(ngram_max * (rand()/(RAND_MAX + 1.0))),
    add_to_idx = first ? 0 : num_strings;
  double shard_prob = 5 * (rand()/(RAND_MAX + 1.0));
  shard_prob /= num_strings;
  FarType far_type = FAR_STLIST;
  if (order < 2) order = 2;   // minimum bigram
  if (!first)
    (*varstrm) << "ORDER" << "=" << order << endl;
  FarWriter<StdArc>* far_writer1;
  NGramCounter<Log64Weight> ngram_counter(order, 0);
  RandGenOptions< NGramArcSelector<StdArc> > opts(*selector, max_length, 1, 0, 0);
  for (int stringidx = 1; stringidx <= num_strings; ++stringidx) {
    VectorFst< StdArc > ofst;
    while (ofst.NumStates() == 0)  // counting only non-empty strings
      RandGen(*genmodel, &ofst, opts);  // Randomly generate one string from model
    ngram_counter.Count(ofst);  // Count n-grams of random order
    if (stringidx == 1 || (rand()/(RAND_MAX + 1.0)) < shard_prob) {
      if (stringidx > 1)  // new random shard, delete old far_writer
	delete far_writer1;
      far_writer1 = FarWriter<StdArc>::Create(directory + far_name(far_num++), 
					      far_type);
    }
    AddToFar(&ofst, key_size, stringidx + add_to_idx, far_writer0);  // all strings
    AddToFar(&ofst, key_size, stringidx, far_writer1);  // string shard
  }

  ngram_counter.GetFst(countfst);  // Get associated count Fst
  ArcSort(countfst, StdILabelCompare());
  countfst->SetInputSymbols(genmodel->InputSymbols());
  countfst->SetOutputSymbols(genmodel->InputSymbols());
  delete far_writer1;
  return far_num;
}

// Make an n-gram model from the count file, using random smoothing method
StdMutableFst *RandomMake(StdMutableFst *countfst) {
  int switchval = ceil(4 * (rand()/(RAND_MAX + 1.0)));  // random method
  if (switchval == 1) {
    NGramKneserNey ngram(countfst, 0, 0, kNormEps, 1, -1, -1);
    ngram.MakeNGramModel();
    return ngram.GetMutableFst();
  } else if (switchval == 2) {
    NGramAbsolute ngram(countfst, 0, 0, kNormEps, 1, -1, -1);
    ngram.MakeNGramModel();
    return ngram.GetMutableFst();
  } else if (switchval == 3) {
    NGramKatz ngram(countfst, 0, 0, kNormEps, 1, -1);
    ngram.MakeNGramModel();
    return ngram.GetMutableFst();
  } else {
    NGramWittenBell ngram(countfst, 0, 0, kNormEps, 1, 1);
    ngram.MakeNGramModel();
    return ngram.GetMutableFst();
  }
}

int main(int argc, char **argv) {
  using fst::StdFst;
  using fst::StdVectorFst;
  using fst::RandGenOptions;
  using fst::RandGen;

  using fst::StdArc;
  using ngram::NGramArcSelector;
  using ngram::NGramCounter;
  using ngram::NGramWittenBell;
  using ngram::NGramOutput;

  string usage = "Generates random data/models.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options]\n";
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  ostream *varstrm = (FLAGS_vars == "") ? &std::cout :
    new ofstream(FLAGS_vars.c_str());

  (*varstrm) << "SEED=" << FLAGS_seed << endl;
  VLOG(0) << "Random Test Seed = " << FLAGS_seed;  // Always show the seed
  // set output directory and seed-based file names
  string directory = directory_label(FLAGS_seed, FLAGS_directory);
  ostream *cntxstrm = new ofstream((directory + "cntxs").c_str());
  FarType far_type = FAR_STLIST;
  FarWriter<StdArc>* far_writer =
    FarWriter<StdArc>::Create(directory + "tocount.far", far_type);
  NGramArcSelector<StdArc> selector(FLAGS_seed);
  for (int i = 0; i < 10; i++)
    rand();  // rand burn in, for improved randomness;

  VectorFst< StdArc > unigram;  // initial random unigram model
  BuildRandomUnigram(&unigram, FLAGS_vocabulary_max, FLAGS_mean_length,
		     varstrm, cntxstrm);
  VectorFst< StdArc > countfst1;  // n-gram counts from random corpus
  double num_samples = FLAGS_sample_max * (rand()/(RAND_MAX + 1.0));
  int num_strings = ceil(num_samples);
  int far_cnt = CountFromRandGen(&unigram, &countfst1, &selector, num_strings,
				 far_writer, 0, FLAGS_max_length, 
				 FLAGS_ngram_max, directory, 1, varstrm);
  // copy first count file, since making model modifies input counts
  StdMutableFst *modfst1 = RandomMake(&countfst1);  // model from counts

  VectorFst< StdArc > countfst2; // n-gram counts from 2nd random corpus
  CountFromRandGen(modfst1, &countfst2, &selector, num_strings,
		   far_writer, far_cnt, FLAGS_max_length, FLAGS_ngram_max, 
		   directory, 0, varstrm);
  delete far_writer;
  if (varstrm != &std::cout) {
    delete varstrm;
  }
  delete cntxstrm;
  return 0;
}
