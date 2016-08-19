
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
// Applies smoothed marginalization constraints to given model.

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <ngram/ngram-marginalize.h>
#include <ngram/util.h>

DEFINE_int64(backoff_label, 0, "Backoff label");
DEFINE_int32(iterations, 1, "Iterations of steady state prob calculation");
DEFINE_int32(max_bo_updates, 10, "Max iterations of backoff re-calculation");
DEFINE_double(norm_eps, ngram::kNormEps, "Normalization check epsilon");
DEFINE_bool(check_consistency, false, "Check model consistency");
DEFINE_bool(output_each_iteration, false, "Output model after each iteration");
DEFINE_string(steady_state_file, "", "Read steady state probs from file");

// One iteration of update to the model
int UpdateIteration(string in_name, string out_name, int64 backoff_label,
                    double norm_eps, int32 max_bo_updates,
                    bool check_consistency, bool output_each_iteration,
                    std::vector<double> *weights, int iter, int tot, int ftot) {
  std::unique_ptr<fst::StdVectorFst> fst(
      fst::StdVectorFst::Read(in_name));
  if (!fst) return -1;

  ngram::NGramMarginal ngramarg(fst.get(), backoff_label, norm_eps,
                                max_bo_updates, check_consistency);

  bool finish = ngramarg.MarginalizeNGramModel(weights, iter, tot);
  if (ngramarg.Error()) return -1;

  if (ftot < 1 && !finish)
    tot++;  // keep going
  else if (ftot < 1)
    tot = iter;  // converged, all done

  if (output_each_iteration) {
    std::ostringstream idxlabel;
    idxlabel << iter;
    VLOG(1) << "Writing: " << out_name << ".iter" << idxlabel.str();
    ngramarg.GetFst().Write(out_name + ".iter" + idxlabel.str());
  } else if (iter == tot) {  // if last iteration, write out model
    ngramarg.GetFst().Write(out_name);
  }
  return tot;
}

int main(int argc, char **argv) {
  string usage = "Marginalize ngram model from input model file.\n\n  Usage: ";
  usage += argv[0];
  usage += " [--options] [in.fst [out.fst]]\n";
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  if (FLAGS_iterations != 1 && (argc < 2 || (strcmp(argv[1], "-") == 0))) {
    NGRAMERROR() << "For " << FLAGS_iterations
                 << " iterations, in.fst argument required" << std::endl
                 << "Use --help for info";
    return 1;
  }
  if (FLAGS_output_each_iteration && FLAGS_iterations == 1)
    FLAGS_output_each_iteration = false;  // just one iteration, no name change
  if (FLAGS_output_each_iteration &&
      (argc < 3 || (strcmp(argv[2], "-") == 0))) {
    LOG(WARNING) << "No output file name given, cannot output each iteration."
                 << std::endl
                 << "Only final model will be produced.";
    FLAGS_output_each_iteration = false;
  }

  std::vector<double> weights;
  if (!FLAGS_steady_state_file.empty()) {  // separate model giving marginals
    std::unique_ptr<fst::StdVectorFst> ssfst(
        fst::StdVectorFst::Read(FLAGS_steady_state_file));
    if (!ssfst) return 1;
    ngram::NGramModel<fst::StdArc> ngram(*ssfst, 0, ngram::kNormEps, true);
    ngram.CalculateStateProbs(&weights, true);
  }

  string in_name = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  string out_name = argc > 2 ? argv[2] : "";

  int iterations = (FLAGS_iterations < 1) ? 2 : FLAGS_iterations;
  for (int i = 1; i <= iterations; i++) {
    VLOG(1) << "Iteration #" << i;
    iterations = UpdateIteration(
        in_name, out_name, FLAGS_backoff_label, FLAGS_norm_eps,
        FLAGS_max_bo_updates, FLAGS_check_consistency,
        FLAGS_output_each_iteration, &weights, i, iterations, FLAGS_iterations);
    if (iterations < 0) return 1;  // fst failed to load or other error.
  }
  return 0;
}
