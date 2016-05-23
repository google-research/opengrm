#!/bin/bash
# Description:
# Tests using fractional count representations with distributed training.

FRAC_DIST=true;
distname=ngramfracdist
export distname
source "$srcdir/disttestsetup.sh" || exit 1

# checks distributed estimation
distributed_test --otype=lm

echo PASS
