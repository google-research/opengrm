#!/bin/bash
# Description:
# Tests distributed training of language models.

distname=ngrammakedist
export distname
source "$srcdir/disttestsetup.sh" || exit 1

# checks distributed estimation
distributed_test --otype=lm --smooth_method=katz
distributed_test --otype=lm --smooth_method=absolute

echo PASS
