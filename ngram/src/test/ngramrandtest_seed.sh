#! /bin/sh

bin=../bin
tmpdata=${TMPDIR:-/tmp}
inputseed=$1
varfile=$tmpdata/ngramrandtest.vars
VERBOSE="--verbose"

set -e

# runs random test, outputs various count and model files and variables
./ngramrandtest --seed=$inputseed -directory="$tmpdata" --vars="$varfile"
. $varfile  # read in variables from rand test (SEED, ORDER...)

./ngramdistrand.sh $SEED $ORDER $VERBOSE

