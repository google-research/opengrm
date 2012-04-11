#! /bin/sh

bin=../bin
testdata=$srcdir/testdata
tmpdata=${TMPDIR:-/tmp}

trap "rm -f $tmpdata/testrand*" 0 2 13 15

set -e

# use random generation to get number of trials
$bin/ngramrandgen --max_sents=100 $testdata/init.randcorpus.0.mod \
 >$tmpdata/testrand.0.params.far
farprintstrings $tmpdata/testrand.0.params.far | grep . | tail -1 | \
 sed 's/ .*//g' | while read i; do z="$(expr 3 \* $i)"; s="$(expr 2 \* $z)";
  echo "TRIALS=$z"; echo "SAMPLES=$s"; done >$tmpdata/testrand.params
. $tmpdata/testrand.params

echo "  Running $TRIALS random trials...";

# get sufficient random strings for different random trial configurations
$bin/ngramrandgen --max_sents=$SAMPLES $testdata/init.randcorpus.0.mod \
 >$tmpdata/testrand.0.params.far
$bin/ngramrandgen --max_sents=$SAMPLES $testdata/init.randcorpus.1.mod \
 >$tmpdata/testrand.1.params.far
$bin/ngramrandgen --max_sents=$SAMPLES $testdata/init.randcorpus.2.mod \
 >$tmpdata/testrand.2.params.far
$bin/ngramrandgen --max_sents=$SAMPLES $testdata/init.randcorpus.3.mod \
 >$tmpdata/testrand.3.params.far

V=1 # whether vocabulary size should come from small (1) or medium (2) size
S=1 # whether number of sentences should come from small/medium/large

# generate param files for each random trial
set +e
i=0
while [ $i -lt $TRIALS ]; do
  i="$(expr $i + 1)"
  printf "V=%s; " "$V" >$tmpdata/testrand.params.$i
  printf "S=%s; " "$S" >>$tmpdata/testrand.params.$i
  farprintstrings $tmpdata/testrand.$V.params.far | grep " .* " | tail -$i | \
  head -1 | while read j k l; do printf "sents1=%s; " "$k"; done \
    >>$tmpdata/testrand.params.$i
  farprintstrings $tmpdata/testrand.$S.params.far | grep . | tail -$i | \
  head -1 | sed 's/.* //g' | while read j; do printf "sents2=%s; " "$j"; done \
    >>$tmpdata/testrand.params.$i
  farprintstrings $tmpdata/testrand.0.params.far | grep " .* .* .* " | \
  tail -$i | head -1 | sed 's/ [^ ]* [^ ]* [^ ]*$/~&/g' | sed 's/.*~ //g' | \
  while read a b c d; do z="$(expr $c - 1)"; y="$(expr $a - 1)";
    if [ $y -gt 0 ]; then printf "O1=%s; " "$y"; else printf "O1=%s; " "$a"; fi;

    printf "O2=%s; " "$b"; echo "T=$z"; done >>$tmpdata/testrand.params.$i
  if [ $S = 2 ]; then S=1; else S=2; fi;  # alternates V={1,2,3}, S={2,3}
  if [ $V -lt 3 ]; then V="$(expr $V + 1)"; else V=1; fi;
  seed1="$RANDOM"
  $bin/ngramrandgen --seed="$seed1" "$testdata/init.randcorpus.0.mod" \
    $tmpdata/testrand.seedrand.far
  seed2="$RANDOM"
  $bin/ngramrandgen --seed="$seed2" $testdata/init.randcorpus.0.mod \
    $tmpdata/testrand.seedrand.far
  echo "seed1=$seed1;seed2=$seed2" >>$tmpdata/testrand.params.$i
done

set -e
# run random trials for each generated param file
i=0
while [ "$i" -lt "$TRIALS" ]; do
  i="$(expr $i + 1)"
  printf "%s params: " "$i"
  ./ngramrand_params.sh $tmpdata/testrand.params.$i $testdata $tmpdata
done
