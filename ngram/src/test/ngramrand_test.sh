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
  echo "V=$V" >$tmpdata/testrand.params.$i
  echo "S=$S" >>$tmpdata/testrand.params.$i
  farprintstrings $tmpdata/testrand.$V.params.far | grep " .* " | tail -$i | \
  head -1 | while read j k l; do echo "sents1=$k"; done \
    >>$tmpdata/testrand.params.$i
  farprintstrings $tmpdata/testrand.$S.params.far | grep . | tail -$i | \
  head -1 | sed 's/.* //g' | while read j; do echo "sents2=$j"; done \
    >>$tmpdata/testrand.params.$i
  farprintstrings $tmpdata/testrand.0.params.far | grep " .* .* .* " | \
  tail -$i | head -1 | sed 's/ [^ ]* [^ ]* [^ ]*$/~&/g' | sed 's/.*~ //g' | \
  while read a b c d; do z="$(expr $c - 1)"; y="$(expr $a - 1)";
    if [ $y -gt 0 ]; then echo "O1=$y"; else echo "O1=$a"; fi; echo "O2=$b"; 
    echo "T=$z"; done >>$tmpdata/testrand.params.$i
  if [ $S = 2 ]; then S=1; else S=2; fi;  # alternates V={1,2,3}, S={2,3}
  if [ $V -lt 3 ]; then V="$(expr $V + 1)"; else V=1; fi; 
done

set -e
# run random trials
i=0
while [ "$i" -lt "$TRIALS" ]; do
  i="$(expr $i + 1)"
  . $tmpdata/testrand.params.$i
  $bin/ngramrandgen --max_sents=$sents1 $testdata/init.randcorpus.$V.mod \
   >$tmpdata/testrand.corp0.far
  farprintstrings $tmpdata/testrand.corp0.far >$tmpdata/testrand.corp0.txt
  $bin/ngramsymbols $tmpdata/testrand.corp0.txt $tmpdata/testrand.syms
  farcompilestrings -symbols=$tmpdata/testrand.syms -keep_symbols=1 \
   $tmpdata/testrand.corp0.txt |\
  $bin/ngramcount -order=$O1 - | \
  $bin/ngrammake --check_consistency >$tmpdata/testrand.m1
  $bin/ngramprint --check_consistency --ARPA $tmpdata/testrand.m1 \
    >$tmpdata/testrand.m1.arpa
  $bin/ngramrandgen --max_sents=$sents2 \
    $tmpdata/testrand.m1 >$tmpdata/testrand.corpus.far
  $bin/ngramcount -order=$O2 $tmpdata/testrand.corpus.far | \
    $bin/ngrammake --check_consistency |\
    $bin/ngramshrink --check_consistency -theta=$T >$tmpdata/testrand.m2.mod
  $bin/ngramprint --check_consistency --ARPA $tmpdata/testrand.m2.mod \
    >$tmpdata/testrand.m2.mod.arpa
  $bin/ngramread --symbols=$tmpdata/testrand.syms --ARPA \
      $tmpdata/testrand.m2.mod.arpa | \
    $bin/ngramprint --check_consistency --ARPA | \
    cmp - $tmpdata/testrand.m2.mod.arpa
  $bin/ngramread --ARPA $tmpdata/testrand.m1.arpa | \
    $bin/ngrammerge --check_consistency - $tmpdata/testrand.m2.mod | \
    $bin/ngramprint --check_consistency --ARPA - | \
    $bin/ngramread --ARPA | \
    $bin/ngramapply - $tmpdata/testrand.corp0.far >$tmpdata/testrand.apply.far;
done
