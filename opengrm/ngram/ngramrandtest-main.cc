// Copyright 2026 The OpenGrm Authors.
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

// Generates random sentences from an LM or more generally paths through any
// FST where epsilons are treated as failure transitions.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "openfst/compat/init.h"
#include "openfst/compat/file_path.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/log.h"
#include "absl/random/bit_gen_ref.h"
#include "absl/random/random.h"
#include "absl/strings/str_cat.h"
#include "openfst/extensions/far/far-type.h"
#include "openfst/extensions/far/far-writer.h"
#include "openfst/extensions/far/far.h"
#include "openfst/lib/arc.h"
#include "openfst/lib/arcsort.h"
#include "openfst/lib/float-weight.h"
#include "openfst/lib/mutable-fst.h"
#include "openfst/lib/randgen.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/vector-fst.h"
#include "opengrm/ngram/ngram-absolute.h"
#include "opengrm/ngram/ngram-count.h"
#include "opengrm/ngram/ngram-katz.h"
#include "opengrm/ngram/ngram-kneser-ney.h"
#include "opengrm/ngram/ngram-model.h"
#include "opengrm/ngram/ngram-randgen.h"
#include "opengrm/ngram/ngram-witten-bell.h"

ABSL_DECLARE_FLAG(int32_t, max_length);
ABSL_DECLARE_FLAG(std::optional<uint64_t>, seed);
ABSL_DECLARE_FLAG(int32_t, vocabulary_max);
ABSL_DECLARE_FLAG(int32_t, mean_length);
ABSL_DECLARE_FLAG(int32_t, sample_max);
ABSL_DECLARE_FLAG(int32_t, ngram_max);
ABSL_DECLARE_FLAG(std::string, directory);
ABSL_DECLARE_FLAG(std::string, vars);
ABSL_DECLARE_FLAG(double, thresh_max);

namespace {

// Builds random context splits over given interval
int BuildContexts(absl::BitGenRef bit_gen, int start, int end, int max,
                  std::ostream& cntxstrm) {
  int split = 0;
  while (split <= 0) {
    split = floor(absl::Uniform(bit_gen, 0.0, static_cast<double>(max)));
  }
  if (split <= start || split > end) {
    cntxstrm << start << " : " << end << std::endl;
  } else {
    BuildContexts(bit_gen, start, split, max, cntxstrm);
    if (end > split) BuildContexts(bit_gen, split, end, max, cntxstrm);
  }
  return split;
}

// Builds a random unigram model based on a maximum vocabulary size and
// a maximum mean length of strings
void BuildRandomUnigram(absl::BitGenRef bit_gen, fst::StdMutableFst* unigram,
                        int vocabulary_max, int mean_length,
                        std::ostream& cntxstrm) {
  int vocabulary =
      ceil(absl::Uniform(bit_gen, 0.0, static_cast<double>(vocabulary_max)));
  double mean_sent_length =
      absl::Uniform(bit_gen, 0.0, static_cast<double>(mean_length));
  BuildContexts(bit_gen, 0, vocabulary + 1, vocabulary + 1, cntxstrm);
  fst::SymbolTable syms;                   // dummy symbol table
  syms.AddSymbol("0");                     // add to dummy symbol table
  unigram->SetStart(unigram->AddState());  // single state automaton
  double C = 0, counts = -log(C);
  std::vector<double> weights;
  for (int i = 0; i < vocabulary; ++i) {  // for each word in vocabulary
    std::ostringstream idxlabel;
    idxlabel << i + 1;
    syms.AddSymbol(idxlabel.str());
    // Random -log count for unigram.
    // TODO: Should this use fst::WeightGenerate<fst::StdArc::Weight>?
    weights.push_back(-log(absl::Uniform(bit_gen, 1.0, 0x1.0p32)));
    counts = ngram::NegLogSum(counts, weights[i], &C);  // for normalization
  }
  double final_cost = counts + log(mean_sent_length);  // how often </s> occurs
  counts = ngram::NegLogSum(counts, final_cost, &C);
  std::vector<double> wts;
  unigram->SetFinal(0, fst::StdArc::Weight(final_cost - counts));  // final cost
  wts.push_back(final_cost - counts);
  for (size_t a = 0; a < vocabulary; ++a) {  // add unigram arcs to model
    unigram->AddArc(0, fst::StdArc(a + 1, a + 1, weights[a] - counts, 0));
    wts.push_back(weights[a] - counts);
  }
  unigram->SetInputSymbols(&syms);
  unigram->SetOutputSymbols(&syms);
}

// Sets up filenames for dumping randomly generated counts and models
std::string directory_label(std::optional<uint64_t> seed, std::string dir) {
  std::string suffix =
      absl::StrCat(seed.has_value() ? absl::StrCat(*seed) : "none", ".");
  return dir.empty() ? suffix : fst::JoinPath(dir, suffix);
}

// Sets up filenames for shard far files
std::string far_name(int32_t far_num) {
  std::ostringstream far_label;
  far_label << far_num;
  std::string far_num_name = "tocount.far." + far_label.str();
  return far_num_name;
}

// Adds Fst to FAR archive file
inline void AddToFar(fst::MutableFst<fst::StdArc>* stringfst, int key_size,
                     int stringkey, fst::FarWriter<fst::StdArc>* far_writer) {
  std::ostringstream keybuf;
  keybuf.width(key_size);
  keybuf.fill('0');
  keybuf << stringkey;
  far_writer->Add(keybuf.str(), *stringfst);
}

// using an input model, generate a random corpus and count n-grams
int CountFromRandGen(absl::BitGenRef bit_gen, fst::StdMutableFst* genmodel,
                     fst::StdMutableFst* countfst,
                     ngram::NGramArcSelector<fst::StdArc>* selector,
                     int num_strings, fst::FarWriter<fst::StdArc>* far_writer0,
                     int in_far_num, int max_length, int ngram_max,
                     const std::string& directory, bool first,
                     std::ostream& varstrm) {
  int key_size = ceil(log10(2 * num_strings)) + 1, far_num = in_far_num,
      order = ceil(absl::Uniform(bit_gen, 0.0, static_cast<double>(ngram_max))),
      add_to_idx = first ? 0 : num_strings;
  double shard_prob = absl::Uniform(bit_gen, 0.0, 5.0 / num_strings);
  fst::FarType far_type = fst::FarType::STLIST;
  if (order < 2) order = 2;  // minimum bigram
  if (!first) varstrm << "ORDER=" << order << std::endl;
  std::unique_ptr<fst::FarWriter<fst::StdArc>> far_writer1;
  ngram::NGramCounter<fst::Log64Weight> ngram_counter(order, false);
  fst::RandGenOptions<ngram::NGramArcSelector<fst::StdArc>> opts(
      *selector, max_length, 1, false, false);
  for (int stringidx = 1; stringidx <= num_strings; ++stringidx) {
    fst::StdVectorFst ofst;
    while (ofst.NumStates() == 0) {  // counting only non-empty strings
      // Randomly generate one string from model
      fst::RandGen(*genmodel, &ofst, opts, bit_gen);
    }
    ngram_counter.Count(ofst);  // Count n-grams of random order
    if (stringidx == 1 || absl::Bernoulli(bit_gen, shard_prob)) {
      far_writer1.reset(fst::FarWriter<fst::StdArc>::Create(
          directory + far_name(far_num++), far_type));
    }
    AddToFar(&ofst, key_size, stringidx + add_to_idx,
             far_writer0);                                    // all strings
    AddToFar(&ofst, key_size, stringidx, far_writer1.get());  // string shard
  }

  ngram_counter.GetFst(countfst);  // Get associated count FST.
  fst::ArcSort(countfst, fst::StdILabelCompare());
  countfst->SetInputSymbols(genmodel->InputSymbols());
  countfst->SetOutputSymbols(genmodel->InputSymbols());
  return far_num;
}

// Make an n-gram model from the count file, using random smoothing method
fst::StdMutableFst* RandomMake(absl::BitGenRef bit_gen,
                               fst::StdMutableFst* countfst) {
  int switchval = ceil(absl::Uniform(bit_gen, 0.0, 4.0));
  if (switchval == 1) {
    ngram::NGramKneserNey ngram(countfst, false, 0, ngram::kNormEps, true, -1,
                                -1);
    ngram.MakeNGramModel();
    return ngram.GetMutableFst();
  } else if (switchval == 2) {
    ngram::NGramAbsolute ngram(countfst, false, 0, ngram::kNormEps, true, -1,
                               -1);
    ngram.MakeNGramModel();
    return ngram.GetMutableFst();
  } else if (switchval == 3) {
    ngram::NGramKatz<fst::StdArc> ngram(countfst, false, 0, ngram::kNormEps,
                                        true, -1);
    ngram.MakeNGramModel();
    return ngram.GetMutableFst();
  } else {
    ngram::NGramWittenBell ngram(countfst, false, 0, ngram::kNormEps, true, 1);
    ngram.MakeNGramModel();
    return ngram.GetMutableFst();
  }
}

}  // namespace

int ngramrandtest_main(int argc, char** argv) {
  std::string usage = "Generates random data/models.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options]\n";
  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);

  std::ofstream varfstrm;
  if (!absl::GetFlag(FLAGS_vars).empty()) {
    varfstrm.open(absl::GetFlag(FLAGS_vars));
    if (!varfstrm) {
      LOG(ERROR) << argv[0]
                 << ": Open failed, file = " << absl::GetFlag(FLAGS_vars);
      return 1;
    }
  }
  std::ostream& varstrm = varfstrm.is_open() ? varfstrm : std::cout;

  std::optional<uint64_t> seed = absl::GetFlag(FLAGS_seed);
  varstrm << "SEED=" << (seed.has_value() ? absl::StrCat(*seed) : "none")
          << std::endl;
  VLOG(0) << "Random Test Seed = "
          << (seed.has_value() ? absl::StrCat(*seed)
                               : "none");  // Always show the seed
  // set output directory and seed-based file names
  std::string directory = directory_label(seed, absl::GetFlag(FLAGS_directory));
  std::ofstream cntxfstrm;
  cntxfstrm.open(directory + "cntxs");
  if (!cntxfstrm) {
    LOG(ERROR) << argv[0] << ": Open failed, file = " << directory << "cntxs";
    return 1;
  }
  std::ostream& cntxstrm = cntxfstrm;
  fst::FarType far_type = fst::FarType::STLIST;
  std::unique_ptr<fst::FarWriter<fst::StdArc>> far_writer(
      fst::FarWriter<fst::StdArc>::Create(directory + "tocount.far", far_type));
  ngram::NGramArcSelector<fst::StdArc> selector;

  absl::BitGen bit_gen;

  fst::StdVectorFst unigram;  // initial random unigram model
  BuildRandomUnigram(bit_gen, &unigram, absl::GetFlag(FLAGS_vocabulary_max),
                     absl::GetFlag(FLAGS_mean_length), cntxstrm);
  fst::StdVectorFst countfst1;  // n-gram counts from random corpus
  double num_samples = absl::Uniform(
      bit_gen, 0.0, static_cast<double>(absl::GetFlag(FLAGS_sample_max)));
  int num_strings = ceil(num_samples);
  int far_cnt = CountFromRandGen(
      bit_gen, &unigram, &countfst1, &selector, num_strings, far_writer.get(),
      0, absl::GetFlag(FLAGS_max_length), absl::GetFlag(FLAGS_ngram_max),
      directory, true, varstrm);
  // copy first count file, since making model modifies input counts
  fst::StdMutableFst* modfst1 = RandomMake(bit_gen, &countfst1);

  fst::StdVectorFst countfst2;  // n-gram counts from 2nd random corpus
  CountFromRandGen(bit_gen, modfst1, &countfst2, &selector, num_strings,
                   far_writer.get(), far_cnt, absl::GetFlag(FLAGS_max_length),
                   absl::GetFlag(FLAGS_ngram_max), directory, false, varstrm);
  return 0;
}
