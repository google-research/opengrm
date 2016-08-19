#!/bin/bash
# Description:
# Random test of ngram functionality.
# Can set environment variable NGRAMRANDTRIALS, otherwise defaults to 0 trials.
# When called with an integer argument, will run that many trials, otherwise
# the default number as explained above.

bin=../bin
tmpdata=${TMPDIR:-/tmp}
tmpsuffix="$(mktemp -u XXXXXXXX 2>/dev/null)"
tmpprefix="${tmpdata}/ngramrand-$tmpsuffix-$RANDOM-$$"
DEFTRIALS=${NGRAMRANDTRIALS:-0}
TRIALS=${1:-$DEFTRIALS}
varfile="$tmpprefix"/ngramrandtest.vars

mkdir -p "${tmpprefix}"

trap "rm -rf ${tmpprefix}" 0 2 13 15

set -e

i=0

while [ "$i" -lt "$TRIALS" ]; do
  : $(( i = $i + 1 ))
  rm -rf "$tmpprefix"/ngramrandtest.*

  # runs random test, outputs various count and model files and variables
./ngramrandtest -directory="$tmpprefix" --vars="$varfile"

  # read in variables from rand test (SEED, ORDER ...)
  . "$varfile"

./ngramdistrand.sh "$SEED" "$ORDER"

  rm -rf "$tmpprefix"/"$SEED".*
  rm -rf "$tmpprefix"/ngramrandtest.*
done

echo PASS
