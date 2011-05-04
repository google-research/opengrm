AM_CPPFLAGS = -I$(srcdir)/../include $(ICU_FLAGS)
## /usr/local/lib/fst is needed for libfstfar
## TODO(rws): investigate why this -rpath $(libdir) is necessary. It should not be:
AM_LDFLAGS = -L/usr/local/lib/fst -lm -ldl -lfst -lfstfar $(ICU_LIBS) -rpath $(libdir)

if HAVE_BIN
bin_PROGRAMS = thraxcompiler fsts-from-far thraxrewrite-tester

thraxcompiler_SOURCES = compiler.cc
thraxcompiler_LDADD = ../lib/libthrax.la

fsts_from_far_SOURCES = fsts-from-far.cc
fsts_from_far_LDADD = ../lib/libthrax.la

thraxrewrite_tester_SOURCES = rewrite-tester.cc
thraxrewrite_tester_LDADD = ../lib/libthrax.la
endif

EXTRA_DIST = thraxmakedep

install-exec-local: $(EXTRA_DIST)
	-mkdir -p -m 755 $(bindir)
	-cp -p $(srcdir)/thraxmakedep $(bindir)

uninstall-local:
	-rm -f $(bindir)/thraxmakedep
