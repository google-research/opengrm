#!/bin/bash
# Description:
# Random test of ngram functionality.
# Can set environment variable NGRAMRANDTRIALS, otherwise defaults to 0 trials.
# When called with an integer argument, will run that many trials, otherwise
# the default number as explained above.

bin=../bin
tmpdata=${TMPDIR:-/tmp}
DEFTRIALS=${NGRAMRANDTRIALS:-0}
TRIALS=${1:-$DEFTRIALS}
varfile="$tmpdata"/ngramrandtest.vars

set -e

i=0

while [ "$i" -lt "$TRIALS" ]; do
  : $(( i = $i + 1 ))
  rm -rf "$tmpdata"/ngramrandtest.*

  # runs random test, outputs various count and model files and variables
./ngramrandtest -directory="$tmpdata" --vars="$varfile"

  # read in variables from rand test (SEED, ORDER ...)
  . "$varfile"

./ngramdistrand.sh "$SEED" "$ORDER"

  rm -rf "$tmpdata"/"$SEED".*
  rm -rf "$tmpdata"/ngramrandtest.*
done

echo PASS
