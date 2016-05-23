#!/bin/bash
# Description:
# Tests distributed training of Witten-Bell language models, requiring transfer.

distname=ngramwbdist
export distname
source "$srcdir/disttestsetup.sh" || exit 1

# checks distributed pruning
distributed_test --otype=pruned_lm --smooth_method=witten_bell \
  --shrink_method=relative_entropy --theta=.00015

echo PASS
