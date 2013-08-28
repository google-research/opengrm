AM_CPPFLAGS = -I$(srcdir)/../include
AM_LDFLAGS = -L/usr/local/lib/fst -lfstfar -lfst -lm -ldl

bin_PROGRAMS = ngramrandtest

ngramrandtest_SOURCES = ngramrandtest.cc
ngramrandtest_LDADD = ../lib/libngram.la

dist_check_SCRIPTS = ngramprint_test.sh ngramcount_test.sh \
                     ngrammake_test.sh ngrammerge_test.sh \
                     ngramshrink_test.sh ngraminfo_test.sh \
                     ngramperplexity_test.sh ngramsymbols_test.sh \
                     ngramrandgen_test.sh ngramapply_test.sh \
                     ngramapply_test.sh ngramrand_test.sh \
                     ngramdist_test.sh ngramcompile_randgen_far.sh \
                     ngramrandtest_seed.sh \
                     ngrammarginalize_test.sh ngramdistrand.sh

dist_noinst_DATA = testdata/earnest-absolute.mod.txt \
                   testdata/earnest-absolute.mod.sym \
                   testdata/earnest-count_prune.pru.txt \
                   testdata/earnest-count_prune.pru.sym \
                   testdata/earnest-det.cnts.txt \
                   testdata/earnest-det.cnts.sym \
                   testdata/earnest-fst.cnts.txt \
                   testdata/earnest-fst.cnts.sym \
                   testdata/earnest-katz.marg.iter2.mod.txt \
                   testdata/earnest-katz.marg.iter2.mod.sym \
                   testdata/earnest-katz.marg.mod.txt \
                   testdata/earnest-katz.marg.mod.sym \
                   testdata/earnest-katz.mod.txt \
                   testdata/earnest-katz.mod.sym \
                   testdata/earnest-kneser_ney.mod.txt \
                   testdata/earnest-kneser_ney.mod.sym \
                   testdata/earnest-min.cnts.txt \
                   testdata/earnest-min.cnts.sym \
                   testdata/earnest-relative_entropy.pru.txt \
                   testdata/earnest-relative_entropy.pru.sym \
                   testdata/earnest-seymore.pru.txt \
                   testdata/earnest-seymore.pru.sym \
                   testdata/earnest-unsmoothed.mod.txt \
                   testdata/earnest-unsmoothed.mod.sym \
                   testdata/earnest-witten_bell.mod.txt \
                   testdata/earnest-witten_bell.mod.sym \
                   testdata/earnest.cnt_of_cnts.txt \
                   testdata/earnest.cnt_of_cnts.sym \
                   testdata/earnest.cnts.txt \
                   testdata/earnest.cnts.sym \
                   testdata/earnest.det.txt \
                   testdata/earnest.det.sym \
                   testdata/earnest.fst.txt \
                   testdata/earnest.fst.sym \
                   testdata/earnest.min.txt \
                   testdata/earnest.min.sym \
                   testdata/earnest.mod.txt \
                   testdata/earnest.mod.sym \
                   testdata/earnest.mrg.norm.txt \
                   testdata/earnest.mrg.norm.sym \
                   testdata/earnest.mrg.smooth.norm.txt \
                   testdata/earnest.mrg.smooth.norm.sym \
                   testdata/earnest.mrg.smooth.txt \
                   testdata/earnest.mrg.smooth.sym \
                   testdata/earnest.mrg.txt \
                   testdata/earnest.mrg.sym \
                   testdata/earnest.randgen.txt \
                   testdata/earnest.randgen.sym \
                   testdata/earnest.txt \
                   testdata/earnest.syms \
                   testdata/init.randcorpus.0.mod.txt \
                   testdata/init.randcorpus.0.mod.sym \
                   testdata/init.randcorpus.1.mod.txt \
                   testdata/init.randcorpus.1.mod.sym \
                   testdata/init.randcorpus.2.mod.txt \
                   testdata/init.randcorpus.2.mod.sym \
                   testdata/init.randcorpus.3.mod.txt \
                   testdata/init.randcorpus.3.mod.sym \
                   testdata/earnest.randgen.apply.FSTtxt.tgz \
                   testdata/earnest.randgen.apply.sym \
                   testdata/earnest.arpa \
                   testdata/earnest.info \
                   testdata/earnest.perp \
                   testdata/earnest.cnt.print

TESTS = ngramprint_test.sh ngramcount_test.sh \
        ngrammake_test.sh ngrammerge_test.sh ngramshrink_test.sh \
        ngraminfo_test.sh ngramperplexity_test.sh ngramsymbols_test.sh \
        ngramrandgen_test.sh ngramapply_test.sh ngramdist_test.sh \
        ngramrand_test.sh ngrammarginalize_test.sh
