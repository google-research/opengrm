EXTRA_DIST = byte.grm numbers.grm example.grm ernest.txt
pkgdatadir=$(datarootdir)/thrax/grammars

install-data-local: $(EXTRA_DIST)
	-mkdir -p -m 755 $(pkgdatadir)
	-cp -p $(srcdir)/byte.grm $(pkgdatadir)
	-cp -p $(srcdir)/numbers.grm $(pkgdatadir)
	-cp -p $(srcdir)/example.grm $(pkgdatadir)
	-cp -p $(srcdir)/ernest.txt $(pkgdatadir)

uninstall-local:
	-rm -rf $(pkgdatadir)
