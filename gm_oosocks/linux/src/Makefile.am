## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" 

AM_CFLAGS =\
	 -Wall\
	 -g

lib_LTLIBRARIES = libgm_oosocks_i486.la


libgm_oosocks_i486_la_SOURCES = \
	lib.c                  \
	AutoUnRef.h                  \
	GMLuaModule.h                  \
	gm_oosocks.cpp                  \
	ILuaInterface.h                  \
	ILuaObject.h                  \
	Socket.cpp                  \
	Socket.h

libgm_oosocks_i486_la_LDFLAGS = 

libgm_oosocks_i486_la_LIBADD = 

include_HEADERS = \
	gm_oosocks_i486.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgm_oosocks_i486-0.1.pc

EXTRA_DIST = \
	libgm_oosocks_i486-0.1.pc.in
