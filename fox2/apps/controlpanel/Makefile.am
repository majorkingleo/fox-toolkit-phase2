## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-1.7.la -lm

bin_PROGRAMS = ControlPanel

man_MANS = ControlPanel.1

ICONS = \
controlpanel_gif.gif \
colors_gif.gif \
filebinding_gif.gif \
settings_gif.gif

# icons.h and icons.cpp are checked in; the reswrap rule of the original
# Makefile.am comes back when fox2/utils/reswrap is ported.
ControlPanel_SOURCES = \
icons.h \
icons.cpp \
ControlPanel.h \
ControlPanel.cpp

EXTRA_DIST = \
$(wildcard *.svg) \
$(man_MANS) \
LICENSE \
$(ICONS)
