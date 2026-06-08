"""Simple LM over the Earnest dataset.

Usage:
------
  bazel build -c opt opengrm/pynini/examples:make_chatspeak_lm
  bazel-bin/opengrm/pynini/examples/make_chatspeak_lm
  /var/tmp/earnest.lm
"""

import pathlib
import shutil
import subprocess
import sys
import tempfile

from absl import app
from absl import flags
from absl import logging
from opengrm.pynini import runfiles



_METHOD = flags.DEFINE_string("method", "witten_bell", "Smoothing method")
_ORDER = flags.DEFINE_integer("order", 3, "N-gram order")

_FARCOMPILESTRINGS_PATH = (
    "openfst/extensions/far/farcompilestrings"
)
_NGRAMCOUNT_PATH = "opengrm/ngram/ngramcount"
_NGRAMINFO_PATH = "opengrm/ngram/ngraminfo"
_NGRAMMAKE_PATH = "opengrm/ngram/ngrammake"
_NGRAMSYMBOLS_PATH = "opengrm/ngram/ngramsymbols"
_DEFAULT_INPUT_PATH = "opengrm/ngram/testdata/earnest.txt"


def main(argv) -> None:
  if len(argv) < 2 or len(argv) > 3:
    raise app.UsageError(
        f"Usage: {argv[0]} [--method method] [--order order] [input_corpus] "
        "<output_lm>"
    )

  # Define candidate paths for binaries.
  try:
    # The binaries we invoke are not linked statically. For this scenario,
    # the only API from `resources` that works is `GetARootDirWithAllResources`.
    farcompilestrings = runfiles.resource_path(_FARCOMPILESTRINGS_PATH)
    ngramcount = runfiles.resource_path(_NGRAMCOUNT_PATH)
    ngraminfo = runfiles.resource_path(_NGRAMINFO_PATH)
    ngrammake = runfiles.resource_path(_NGRAMMAKE_PATH)
    ngramsymbols = runfiles.resource_path(_NGRAMSYMBOLS_PATH)
  except (OSError, NotImplementedError):
    logging.error("Failed to find required resources.")
    raise

  if len(argv) == 2:
    # Backward-compatible single-argument mode: only output is specified.
    input_corpus = runfiles.resource_path(_DEFAULT_INPUT_PATH)
    output_lm = pathlib.Path(argv[1])
  else:
    input_corpus = pathlib.Path(argv[1])
    output_lm = pathlib.Path(argv[2])

  with tempfile.TemporaryDirectory(prefix="chatspeak_lm_") as temp_dir_str:
    temp_dir = pathlib.Path(temp_dir_str)
    linput = temp_dir / "earnest.txt"
    syms = temp_dir / "syms"
    far = temp_dir / "far"
    counts = temp_dir / "counts"
    fst = temp_dir / "fst"

    logging.info("TMPDIR=%s", temp_dir)

    logging.info("Lower-casing corpus")
    with open(input_corpus, "r", encoding="utf-8") as infile:
      with open(linput, "w", encoding="utf-8") as outfile:
        for line in infile:
          outfile.write(line.lower())

    logging.info("Creating FAR symbols")
    subprocess.run([ngramsymbols, linput, syms], check=True)

    logging.info("Creating FAR")
    subprocess.run(
        [
            farcompilestrings,
            f"--symbols={syms}",
            "--fst_type=compact",
            "--keep_symbols",
            linput,
            far,
        ],
        check=True,
    )

    logging.info("Collecting counts")
    subprocess.run(
        [
            ngramcount,
            "--require_symbols=false",
            f"--order={_ORDER.value}",
            far,
            counts,
        ],
        check=True,
    )

    logging.info("Normalizing model")
    subprocess.run(
        [ngrammake, f"--method={_METHOD.value}", counts, fst], check=True
    )

    print("N-Gram model info:\n")
    sys.stdout.flush()
    subprocess.run([ngraminfo, fst], check=True)
    print()

    shutil.move(fst, output_lm)
    logging.info("Output FST: %s", output_lm)


if __name__ == "__main__":
  app.run(main)
