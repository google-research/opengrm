AM_CPPFLAGS = -I$(srcdir)/../include

lib_LTLIBRARIES = libngram.la
libngram_la_SOURCES =  ngram-make.cc ngram-merge.cc ngram-model.cc \
	               ngram-output.cc ngram-shrink.cc ngram-kneserney.cc \
                       ngram-katz.cc ngram-absolute.cc \
                       lexicographic-map.cc ngram-countprune.cc
libngram_la_LDFLAGS = -version-info 0:0:0
