#! /bin/sh

bin=../bin
tmpdata=${TMPDIR:-/tmp}
TRIALS=${NGRAMRANDTRIALS:-0}
varfile=$tmpdata/ngramrandtest.vars

set -e

i=0

while [ "$i" -lt "$TRIALS" ]; do
  i="$(expr $i + 1)"
  rm -rf $tmpdata/ngramrandtest.*

  # runs random test, outputs various count and model files and variables
  ./ngramrandtest -directory="$tmpdata" --vars="$varfile"
  . $varfile  # read in variables from rand test (SEED, ORDER ...)

  ./ngramdistrand.sh $SEED $ORDER

  rm -rf $tmpdata/$SEED.*
  rm -rf $tmpdata/ngramrandtest.*
done
