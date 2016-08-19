#!/bin/bash
# Description:
# Random test of ngram functionality with a given input seed.

bin=../bin
inputseed=$1
tmpdata=${TMPDIR:-/tmp}
tmpprefix="${tmpdata}/ngramrand-seed${inputseed}"
varfile="${tmpprefix}/ngramrandtest.vars"
VERBOSE="--verbose"

rm -rf "${tmpprefix}"
mkdir -p "${tmpprefix}"
set -e

# runs random test, outputs various count and model files and variables
./ngramrandtest --seed="$inputseed" -directory="$tmpprefix" \
  --vars="$varfile"
. $varfile  # read in variables from rand test (SEED, ORDER...)

./ngramdistrand.sh $SEED $ORDER $VERBOSE

