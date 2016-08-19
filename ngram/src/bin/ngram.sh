#!/bin/bash
# Description:
# Convenience script for training models in a distributed fashion.

tmpdata=${TMPDIR:-/tmp}
tmpsuffix="$(mktemp -u XXXXXXXX 2>/dev/null)"
dir="${tmpdata}/ngram-$tmpsuffix-$RANDOM-$$"
bin=../bin

trap "rm -fr $dir" 0 2 13 15

ifile=""
ofile=""
itype=""
otype=""
contexts=""
merge_contexts=false
symbols=""
usage=false
verbose=false

# (1) text sents -> FST sents flags
OOV_symbol=""

# (2) FST sents -> counts flags
order=3
epsilon_as_backoff=false
round_to_int=false

# (3) FST counts -> LM flags
smooth_method=katz
witten_bell_k=1
discount_D=-1
bins=-1

# (4) LM -> pruned LM flags
theta=0.0
shrink_method=seymore

updated=false

while [ $# != 0 ]
do
  # Parse 'option=optarg' word
  option="$(awk 'BEGIN { split(ARGV[1], a, "=") ; print a[1] }' $1)"
  optarg="$(awk 'BEGIN { split(ARGV[1], a, "=") ; print a[2] }' $1)"
  shift
  case "$option" in
    --bins|-bins)
      bins="$optarg" ;;
    --contexts|-contexts)
      contexts="$optarg" ;;
    --epsilon_as_backoff|-epsilon_as_backoff)
      epsilon_as_backoff=true ;;
    --round_to_int|-round_to_int)
      round_to_int=true ;;
    --help|-help)
      usage=true ;;
    --ifile|-ifile)
      ifile="$optarg" ;;
    --itype|-itype)
      itype="$optarg"
      if [ "$itype" != text_sents -a "$itype" != fst_sents -a \
          "$itype" != counts -a "$itype" != lm ]
      then
        echo "ERROR: bad input type: \"$itype\""
        exit 1
      fi ;;
    --merge_contexts|-merge_contexts)
      merge_contexts=true ;;
    --ofile|-ofile)
      ofile="$optarg" ;;
    --order|-order)
      order="$optarg" ;;
    --otype|-otype)
      otype="$optarg"
      if [ "$otype" != fst_sents -a "$otype" != counts -a \
        "$otype" != lm -a "$otype" != pruned_lm ]
      then
        echo "ERROR: bad output type: \"$otype\""
        exit 1
      fi ;;
    --shrink_method|-shrink_method)
      shrink_method="$optarg" ;;
    --smooth_method|-smooth_method)
      smooth_method="$optarg" ;;
    --symbols|-symbols)
      symbols="$optarg" ;;
    --OOV_symbol|-OOV_symbol)
      OOV_symbols="$optarg" ;;
    --theta|-theta)
      theta="$optarg" ;;
    --verbose|-verbose)
      verbose=true ;;
    --witten_bell_k|-witten_bell_k)
      witten_bell_k="$optarg" ;;
    *)
      echo "bad option: $option"
      exit 1 ;;
  esac
done

if [ -z "$ifile" -o -z "$ofile" -o -z "$itype" -o -z "$otype" \
     -o "$usage" = true ]
then
   echo "Usage: $0  [--options] --ifile <infile> --ofile <outfile>\
 --itype <input type> --otype <output type>"
   echo
   echo "General flags:"
   echo "  --contexts            context pattern filename"
   echo "  --ifile               input filename pattern"
   echo "  --itype               input format, one of:"
   echo "    \"text_sents\", \"fst_sents\", \"counts\", \"lm\""
   echo "  --merge_contexts      merge_contexts in result"
   echo "  --ofile               output filename pattern"
   echo "  --otype               output format, one of:"
   echo "    \"fst_sents\", \"counts\", \"lm\", \"pruned_lm\""
   echo "  --symbols             symbol_table"
   echo
   echo "Sentence compilation flags"
   echo "  --OOV_symbol          out-of-vocabulary symbol (default: "")"
   echo
   echo "Counting flags:"
   echo "  --epsilon_as_backoff  treat epsilon in the input Fsts as backoff"
   echo "  --round_to_int        Round all counts to integers (for testing)"
   echo "  --order               set maximal order of ngrams to be counted"
   echo
   echo "Smoothing flags:"
   echo "  --bins                no. of bins for katz or absolute discounting"
   echo "  --discount_D          absolute discount value D to use"
   echo "  --smooth_method       one of:"
   echo "    \"absolute\", \"katz\" (default), \"kneser_ney\","
   echo "    \"presmoothed\", \"unsmoothed\", \"witten_bell\""
   echo "  --witten_bell_k       Witten-Bell hyperparameter K (default: 1)"
   echo
   echo "Shrinking flags:"
   echo "  --smooth_method       one of:"
   echo "    \"absolute\", \"seymore\" (default)"
   echo "  --theta               pruning threshold theta"
   echo "  --verbose             show progress"
   echo
   echo "Environment variables:"
   echo "    TMPDIR              working dir for temp results (default:/tmp)"
   exit 1
fi

set -e

message() {
  if [ "$verbose" = true ]
  then
      echo "[$(date)] $1"
  fi
}

# Moves output to input.
# Sets itype to 1st arg (if any).
update() {
  rm -fr "$dir/input"
  mv "$dir/output" "$dir/input"
  mkdir "$dir/output"
  rm -f "$dir"/tmp/*
  if [ -n "$1" ] ; then itype="$1" ; fi
  updated=true
}

# Creates FST sentences.
compile_sentences() {
  message "Compiling text sentences"
  if [ -z "$symbols" ]
  then
    echo "ERROR: symbol table must be provided to compile sentences"
    exit 1
  fi
  for inf in "$dir"/input/*
  do
    outf="$dir"/output/"$(basename $inf)"
    farcompilestrings \
      --symbols="$symbols" \
      --unknown_symbol="$OOV_symbol" \
      --keep_symbols=1 \
      "$inf" "$outf"
  done
  update fst_sents
}

# Splits each data shard by context.
ngram_split() {
  message "Splitting count shards by context"
  for inf in "$dir"/input/*
  do
    outf="$dir"/output/"$(basename $inf).c"
    "${bin}"/ngramsplit --contexts="$contexts" "$inf" "$outf"
  done
  update
}

# Merges all data shards of the same context.
ngram_merge_counts() {
  message "Merging context shards with the same context"
  while read c ignore
  do
    outf="$dir"/output/"c$c"
    "${bin}"/ngrammerge \
      --check_consistency \
      --complete \
      --round_to_int="$round_to_int" \
      --method=count_merge \
      --ofile="$outf" \
      "$dir"/input/d*."c$c"
  done <"$dir/side/idcontexts"
  update
}

# Splits each context shard by context.
ngram_sub_split() {
  message "Splitting contexts shards by context"
  while read c ignore
  do
    inf="$dir"/input/"c$c"
    outf="$dir"/output/"c$c.s"
    "${bin}"/ngramsplit --complete --contexts="$contexts" "$inf" "$outf"
    # Replace diagonal with the original context shard.
    rm -f "$dir"/output/"c$c"."s$c"
    ln "$inf" "$dir"/output/"c$c"."s$c"
  done <"$dir/side/idcontexts"
  update
}

# Transfers from sub-context shards.
# Argument(s) are additional options to ngramtransfer.
ngram_transfer_to() {
  message "Transferring to sub-context shards from context shards."
  while read s j ignore
  do
    outf="$dir"/output/"s$s.c"
    "${bin}"/ngramtransfer "$@" \
      --contexts="$contexts" \
      --transfer_from=false \
      --index="$j" \
      --complete \
      --ofile="$outf" \
      "$dir"/input/c*."s$s"
    # Add diagonal using the original context shard.
    ln "$dir"/input/"c$s"."s$s" "$outf$s"
  done <"$dir/side/idcontexts"
  update
}

# Transfers from sub-context shards.
# Argument(s) are additional options to ngramtransfer.
ngram_transfer_from() {
  message "Transferring from sub-context shards to context shards."
  while read c i ignore
  do
    outf="$dir"/output/"c$c"
    "${bin}"/ngramtransfer "$@" \
      --contexts="$contexts" \
      --transfer_from=true \
      --index="$i" \
      --complete \
      --ofile="$outf" \
      "$dir"/input/s*."c$c"
  done <"$dir/side/idcontexts"
  update
}

# Completes each context shard.
# Argument(s) are additional options to (final) ngramtransfer.
ngram_complete() {
  ngram_sub_split
  ngram_transfer_to
  ngram_transfer_from "$@"
}

# Counts n-grams.
ngram_count() {
  message "Counting n-grams"
  for inf in "$dir"/input/*
  do
    outf="$dir"/output/"$(basename $inf)"
    "${bin}"/ngramcount --order="$order" \
      --epsilon_as_backoff="$epsilon_as_backoff" \
      --round_to_int="$round_to_int" "$inf" "$outf"
  done
  update counts

  if [ -n "$contexts" ]
  then
    awk '{ printf "%05d %d %s\n", (NR-1), (NR-1), $0 }' \
      "$contexts" >"$dir/side/idcontexts"
    ngram_split
    ngram_merge_counts
    ngram_complete
  fi
}

# Computes count of counts.
ngram_count_of_counts() {
  message "Computing count of counts"
  while read c i context
  do
    inf="$dir"/input/"c$c"
    outf="$dir"/tmp/"c$c"
    "${bin}"/ngramcount -method=count_of_counts --context_pattern="$context" \
      "$inf" "$outf"
  done <"$dir/side/idcontexts"
  "${bin}"/ngrammerge \
    --check_consistency \
    --ofile="$dir/side/count_of_counts" \
    --method=count_merge \
    --contexts="$contexts" \
    "$dir"/tmp/c*
  rm -f "$dir"/tmp/c*
}


# Smoothes model.
# Argument(s) are additional options to ngrammake.
ngram_make() {
  message "Smoothing model"
  if [ -n "$contexts" -a "$smooth_method" = kneser_ney ]
  then
    echo "ERROR: \"$smooth_method\" not supported in distributed mode"
    exit 1
  fi

  for inf in "$dir"/input/*
  do
    outf="$dir"/output/"$(basename $inf)"
    "${bin}"/ngrammake "$@" \
      --check_consistency \
      --method="$smooth_method" \
      --witten_bell_k="$witten_bell_k" \
      --discount_D="$discount_D" \
      --bins="$bins" "$inf" "$outf"
  done
  update lm

  if [ -n "$contexts" -a "$smooth_method" = witten_bell ]
  then
    ngram_complete --normalize
  fi
}

# Shrinks model.
ngram_shrink() {
  message "Shrinking model"
  if [ -n "$contexts" ]
  then
    while read c i context
    do
      inf="$dir"/input/"c$c"
      outf="$dir"/output/"c$c"
      "${bin}"/ngramshrink \
        --check_consistency \
        --method="$shrink_method" \
        -context_pattern="$context" \
        --theta="$theta" \
        "$inf" "$outf"
    done <"$dir/side/idcontexts"
  else
    inf="$dir/input/d00000"
    outf="$dir/output/d00000"
    "${bin}"/ngramshrink \
      --check_consistency \
      --method="$shrink_method" \
      --theta="$theta" \
      "$inf" "$outf"
  fi
  update pruned_lm
}

# Merges contexts into single result.
ngram_merge_contexts() {
  message "Merging context shards"
  case "$otype" in
    fst_sents)
      echo "ERROR: bad output type ($otype) for merging contexts"
      exit 1 ;;
    counts)
      unset normalize ;;
    *)
      normalize="--normalize" ;;
  esac
  outf="$dir"/output/merged
  "${bin}"/ngrammerge \
    $normalize \
    --check_consistency \
    --method=context_merge \
    --contexts="$contexts" \
    --ofile="$outf" \
    "$dir"/input/c*
  update
}

run_pipeline() {
   if [ "$itype" = text_sents ]
   then
     compile_sentences
   fi

   if [ "$otype" = fst_sents ] ; then return; fi

   if [ "$itype" = fst_sents ]
   then
     ngram_count
   fi

   if [ "$otype" = counts ] ; then return; fi

   if [ "$itype" = counts ]
   then
     if [ -n "$contexts" ]
     then
       ngram_count_of_counts
       ngram_make --count_of_counts="$dir/side/count_of_counts"
     else
       ngram_make
     fi
   fi

   if [ "$otype" = lm ] ; then return; fi

   if [ "$itype" = lm ]
   then
     ngram_shrink
   fi
}

mkdir "$dir" "$dir/input" "$dir/output" "$dir/side" "$dir/tmp"

# Copies renamed input data to working directory.
message "Copying data to working directory"
j=0;
for inf in $ifile
do
  i="$(awk 'BEGIN { printf "%05d\n", '$j' }' </dev/null)"
  : $(( j = $j + 1 ))
  cp "$inf" "$dir"/input/"d$i"
done

if [ -z "$contexts" -a "$i" -gt 1 ]
then
  echo "ERROR: contexts flag must be specified with multiple input files"
  exit 1
fi

# Processes input.
run_pipeline

if [ "$updated" = false ]
then
  echo "ERROR: bad input type ($itype) for output type ($otype)"
  exit 1
fi

# Returns result.
if [ -z "$contexts" ]
then
  mv "$dir"/input/* "$ofile"
elif [ "$merge_contexts" = true ]
then
  ngram_merge_contexts
  mv "$dir"/input/* "$ofile"
else
  for inf in "$dir"/input/*
  do
    outf="$ofile"."$(basename $inf)"
    mv "$inf" "$outf"
  done
fi

message "Done"
