#!/bin/bash
# Description:
# Tests distributed training and pruning of language models.

distname=ngramshrinkdist
export distname
source "$srcdir/disttestsetup.sh" || exit 1

# checks distributed pruning
distributed_test --otype=pruned_lm --smooth_method=katz \
  --shrink_method=relative_entropy --theta=.00015
distributed_test --otype=pruned_lm --smooth_method=katz \
  --shrink_method=seymore --theta=4

echo PASS
