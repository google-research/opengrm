EXTRA_DIST = regression_test_build_defs.bzl

pkgdatadir=$(DESTDIR)$(datarootdir)/thrax/bazel

install-data-local: $(EXTRA_DIST)
	-mkdir -p -m 755 $(pkgdatadir)
	-cp -p $(srcdir)/regression_test_build_defs.bzl $(pkgdatadir)

uninstall-local:
	-rm -rf $(pkgdatadir)
