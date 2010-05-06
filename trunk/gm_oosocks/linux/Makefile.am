## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src po

gm_oosocks_i486docdir = ${prefix}/doc/gm_oosocks_i486
gm_oosocks_i486doc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS


INTLTOOL_FILES = intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

EXTRA_DIST = $(gm_oosocks_i486doc_DATA) \
	$(INTLTOOL_FILES)

DISTCLEANFILES = intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache


# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(gm_oosocks_i486docdir)
