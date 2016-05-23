#!/bin/bash
# Description:
# Tests using fractional count representations with distributed shrinking.

FRAC_DIST=true;
distname=ngramfracdistshrink
export distname
source "$srcdir/disttestsetup.sh" || exit 1

# checks distributed pruning
distributed_test --otype=pruned_lm --shrink_method=relative_entropy \
  --theta=.00015
distributed_test --otype=pruned_lm --shrink_method=seymore --theta=4

echo PASS
