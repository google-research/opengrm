#!/bin/bash
# Description:
# Tests distributed counting of language models.

distname=ngramcountdist
export distname
source "$srcdir/disttestsetup.sh" || exit 1

# checks distributed counting
distributed_test --otype=counts

echo PASS
