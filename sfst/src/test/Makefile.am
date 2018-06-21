AM_CPPFLAGS = -I$(srcdir)/../include
AM_LDFLAGS = -L/usr/local/lib/fst -lfst -lm -ldl

bin_PROGRAMS = sfst_algo_test

sfst_algo_test_SOURCES = algo_test.cc
sfst_algo_test_LDADD = ../lib/libsfst.la

TESTS = sfst_algo_test
