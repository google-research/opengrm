AM_CPPFLAGS = -I$(srcdir)/../include

lib_LTLIBRARIES = libngram.la
libngram_la_SOURCES = ngram-make.cc ngram-merge.cc ngram-model.cc \
	              ngram-mutable-model.cc ngram-output.cc \
	              ngram-count-of-counts.cc \
		      ngram-shrink.cc ngram-kneser-ney.cc \
		      ngram-katz.cc ngram-absolute.cc \
		      ngram-count-prune.cc ngram-split.cc \
                      ngram-context.cc ngram-marginalize.cc \
                      ngram-complete.cc ngram-transfer.cc
libngram_la_LDFLAGS = -version-info 1:0:0
