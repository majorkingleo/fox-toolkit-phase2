## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-2.0.la -lm

bin_PROGRAMS = PathFinder

if COND_OSWINDOWS
PathFinder_LDFLAGS = -mwindows
endif

man_MANS = PathFinder.1

# icons.h and icons.cpp are checked in; they are the output of reswrap over
# the images below, as in the calculator and control panel ports.
PathFinder_SOURCES = \
icons.h \
icons.cpp \
CopyDialog.h \
CopyDialog.cpp \
PathFinder.cpp \
PathFinder.h \
Preferences.cpp \
Preferences.h \
PropertyDialog.cpp \
PropertyDialog.h

EXTRA_DIST = \
$(man_MANS) \
LICENSE \
$(wildcard *.gif) \
$(wildcard *.bmp)
