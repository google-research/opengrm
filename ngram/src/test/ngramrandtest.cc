
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
// Copyright 2005-2016 Brian Roark and Google, Inc.
// Generates random sentences from an LM or more generally paths through any
// FST where epsilons are treated as failure transitions.

#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include <fst/extensions/far/far.h>
#include <fst/fst.h>
#include <fst/randgen.h>
#include <fst/rmepsilon.h>
#include <fst/shortest-path.h>
#include <fst/vector-fst.h>

#include <ngram/ngram-absolute.h>
#include <ngram/ngram-context-merge.h>
#include <ngram/ngram-count-merge.h>
#include <ngram/ngram-count.h>
#include <ngram/ngram-katz.h>
#include <ngram/ngram-kneser-ney.h>
#include <ngram/ngram-make.h>
#include <ngram/ngram-model-merge.h>
#include <ngram/ngram-output.h>
#include <ngram/ngram-randgen.h>
#include <ngram/ngram-seymore-shrink.h>
#include <ngram/ngram-shrink.h>
#include <ngram/ngram-witten-bell.h>

DEFINE_int32(max_length, 1000, "Maximum sentence length");
DEFINE_int32(seed, time(0) + getpid(), "Randomization seed");
DEFINE_int32(vocabulary_max, 5000, "maximum vocabulary size");
DEFINE_int32(mean_length, 100, "maximum mean string length");
DEFINE_int32(sample_max, 10000, "maximum sample corpus size");
DEFINE_int32(ngram_max, 3, "maximum n-gram order size");
DEFINE_string(directory, ".", "directory where files will be placed");
DEFINE_string(vars, "", "file name for outputting variable values");
DEFINE_double(thresh_max, 3, "maximum threshold size");

// Builds random context splits over given interval
int BuildContexts(int start, int end, int max, std::ostream &cntxstrm) {
  int split = floor(max * (rand() / (RAND_MAX + 1.0)));
  while (split <= 0) split = floor(max * (rand() / (RAND_MAX + 1.0)));
  if (split <= start || split > end) {
    cntxstrm << start << " : " << end << std::endl;
  } else {
    BuildContexts(start, split, max, cntxstrm);
    if (end > split) BuildContexts(split, end, max, cntxstrm);
  }
  return split;
}

// Builds a random unigram model based on a maximum vocabulary size and
// a maximum mean length of strings
void BuildRandomUnigram(fst::StdMutableFst *unigram, int vocabulary_max,
                        int mean_length, std::ostream &cntxstrm) {
  int vocabulary = ceil(vocabulary_max * (rand() / (RAND_MAX + 1.0)));
  double mean_sent_length = mean_length * (rand() / (RAND_MAX + 1.0));
  BuildContexts(0, vocabulary + 1, vocabulary + 1, cntxstrm);
  fst::SymbolTable syms;               // dummy symbol table
  syms.AddSymbol("0");                     // add to dummy symbol table
  unigram->SetStart(unigram->AddState());  // single state automaton
  double C = 0, counts = -log(C);
  std::vector<double> weights;
  for (int i = 0; i < vocabulary; ++i) {  // for each word in vocabulary
    std::ostringstream idxlabel;
    idxlabel << i + 1;
    syms.AddSymbol(idxlabel.str());       // add to dummy symbol table
    weights.push_back(-log(rand() + 1));  // random -log count for unigram
    counts = ngram::NegLogSum(counts, weights[i], &C);  // for normalization
  }
  double final_cost = counts + log(mean_sent_length);  // how often </s> occurs
  counts = ngram::NegLogSum(counts, final_cost, &C);
  std::vector<double> wts;
  unigram->SetFinal(0, final_cost - counts);  // final cost
  wts.push_back(final_cost - counts);
  for (size_t a = 0; a < vocabulary; ++a) {  // add unigram arcs to model
    unigram->AddArc(0, fst::StdArc(a + 1, a + 1, weights[a] - counts, 0));
    wts.push_back(weights[a] - counts);
  }
  unigram->SetInputSymbols(&syms);
  unigram->SetOutputSymbols(&syms);
}

// Sets up filenames for dumping randomly generated counts and models
string directory_label(int32 seed, string dir) {
  std::ostringstream seedlabel;
  seedlabel << seed;
  string directory = (dir == "") ? "" : dir + "/";
  directory += seedlabel.str() + ".";
  return directory;
}

// Sets up filenames for shard far files
string far_name(int32 far_num) {
  std::ostringstream far_label;
  far_label << far_num;
  string far_num_name = "tocount.far." + far_label.str();
  return far_num_name;
}

// Adds Fst to FAR archive file
inline void AddToFar(fst::MutableFst<fst::StdArc> *stringfst,
                     int key_size, int stringkey,
                     fst::FarWriter<fst::StdArc> *far_writer) {
  std::ostringstream keybuf;
  keybuf.width(key_size);
  keybuf.fill('0');
  keybuf << stringkey;
  string key;
  far_writer->Add(keybuf.str(), *stringfst);
}

// using an input model, generate a random corpus and count n-grams
int CountFromRandGen(fst::StdMutableFst *genmodel,
                     fst::StdMutableFst *countfst,
                     ngram::NGramArcSelector<fst::StdArc> *selector,
                     int num_strings,
                     fst::FarWriter<fst::StdArc> *far_writer0,
                     int in_far_num, int max_length, int ngram_max,
                     const string &directory, bool first,
                     std::ostream &varstrm) {
  int key_size = ceil(log10(2 * num_strings)) + 1, far_num = in_far_num,
      order = ceil(ngram_max * (rand() / (RAND_MAX + 1.0))),
      add_to_idx = first ? 0 : num_strings;
  double shard_prob = 5 * (rand() / (RAND_MAX + 1.0));
  shard_prob /= num_strings;
  fst::FarType far_type = fst::FAR_STLIST;
  if (order < 2) order = 2;  // minimum bigram
  if (!first) varstrm << "ORDER=" << order << std::endl;
  std::unique_ptr<fst::FarWriter<fst::StdArc>> far_writer1;
  ngram::NGramCounter<fst::Log64Weight> ngram_counter(order, 0);
  fst::RandGenOptions<ngram::NGramArcSelector<fst::StdArc>> opts(
      *selector, max_length, 1, 0, 0);
  for (int stringidx = 1; stringidx <= num_strings; ++stringidx) {
    fst::StdVectorFst ofst;
    while (ofst.NumStates() == 0)  // counting only non-empty strings
      fst::RandGen(*genmodel, &ofst,
                       opts);   // Randomly generate one string from model
    ngram_counter.Count(ofst);  // Count n-grams of random order
    if (stringidx == 1 || (rand() / (RAND_MAX + 1.0)) < shard_prob) {
      far_writer1.reset(fst::FarWriter<fst::StdArc>::Create(
          directory + far_name(far_num++), far_type));
    }
    AddToFar(&ofst, key_size, stringidx + add_to_idx,
             far_writer0);                              // all strings
    AddToFar(&ofst, key_size, stringidx, far_writer1.get());  // string shard
  }

  ngram_counter.GetFst(countfst);  // Get associated count Fst
  fst::ArcSort(countfst, fst::StdILabelCompare());
  countfst->SetInputSymbols(genmodel->InputSymbols());
  countfst->SetOutputSymbols(genmodel->InputSymbols());
  return far_num;
}

// Make an n-gram model from the count file, using random smoothing method
fst::StdMutableFst *RandomMake(fst::StdMutableFst *countfst) {
  int switchval = ceil(4 * (rand() / (RAND_MAX + 1.0)));  // random method
  if (switchval == 1) {
    ngram::NGramKneserNey ngram(countfst, 0, 0, ngram::kNormEps, 1, -1, -1);
    ngram.MakeNGramModel();
    return ngram.GetMutableFst();
  } else if (switchval == 2) {
    ngram::NGramAbsolute ngram(countfst, 0, 0, ngram::kNormEps, 1, -1, -1);
    ngram.MakeNGramModel();
    return ngram.GetMutableFst();
  } else if (switchval == 3) {
    ngram::NGramKatz<fst::StdArc> ngram(countfst, 0, 0, ngram::kNormEps, 1,
                                            -1);
    ngram.MakeNGramModel();
    return ngram.GetMutableFst();
  } else {
    ngram::NGramWittenBell ngram(countfst, 0, 0, ngram::kNormEps, 1, 1);
    ngram.MakeNGramModel();
    return ngram.GetMutableFst();
  }
}

int main(int argc, char **argv) {
  string usage = "Generates random data/models.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options]\n";
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  std::ofstream varfstrm;
  if (!FLAGS_vars.empty()) {
    varfstrm.open(FLAGS_vars);
    if (!varfstrm) {
      LOG(ERROR) << argv[0] << ": Open failed, file = " << FLAGS_vars;
      return 1;
    }
  }
  std::ostream &varstrm = varfstrm.is_open() ? varfstrm : std::cout;

  varstrm << "SEED=" << FLAGS_seed << std::endl;
  VLOG(0) << "Random Test Seed = " << FLAGS_seed;  // Always show the seed
  // set output directory and seed-based file names
  string directory = directory_label(FLAGS_seed, FLAGS_directory);
  std::ofstream cntxfstrm;
  cntxfstrm.open(directory + "cntxs");
  if (!cntxfstrm) {
    LOG(ERROR) << argv[0] << ": Open failed, file = " << directory << "cntxs";
    return 1;
  }
  std::ostream &cntxstrm = cntxfstrm;
  fst::FarType far_type = fst::FAR_STLIST;
  std::unique_ptr<fst::FarWriter<fst::StdArc>> far_writer(
      fst::FarWriter<fst::StdArc>::Create(directory + "tocount.far",
                                                  far_type));
  ngram::NGramArcSelector<fst::StdArc> selector(FLAGS_seed);
  for (int i = 0; i < 10; i++)
    rand();  // rand burn in, for improved randomness;

  fst::StdVectorFst unigram;  // initial random unigram model
  BuildRandomUnigram(&unigram, FLAGS_vocabulary_max, FLAGS_mean_length,
                     cntxstrm);
  fst::StdVectorFst countfst1;  // n-gram counts from random corpus
  double num_samples = FLAGS_sample_max * (rand() / (RAND_MAX + 1.0));
  int num_strings = ceil(num_samples);
  int far_cnt = CountFromRandGen(&unigram, &countfst1, &selector, num_strings,
                                 far_writer.get(), 0, FLAGS_max_length,
                                 FLAGS_ngram_max, directory, 1, varstrm);
  // copy first count file, since making model modifies input counts
  fst::StdMutableFst *modfst1 =
      RandomMake(&countfst1);  // model from counts

  fst::StdVectorFst countfst2;  // n-gram counts from 2nd random corpus
  CountFromRandGen(modfst1, &countfst2, &selector, num_strings,
                   far_writer.get(), far_cnt, FLAGS_max_length, FLAGS_ngram_max,
                   directory, 0, varstrm);
  return 0;
}
