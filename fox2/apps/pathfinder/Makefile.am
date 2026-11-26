## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-2.0.la -lm
RESWRAP = $(top_builddir)/utils/reswrap$(EXEEXT)

bin_PROGRAMS = PathFinder

if COND_OSWINDOWS
PathFinder_LDFLAGS = -mwindows
endif

man_MANS = PathFinder.1

ICONS = $(wildcard $(srcdir)/*.gif) $(wildcard $(srcdir)/*.bmp)

# icons.h and icons.cpp are generated from the image files by reswrap.
BUILT_SOURCES = icons.h icons.cpp
CLEANFILES = icons.h icons.cpp

icons.h: $(ICONS) $(RESWRAP)
	$(RESWRAP) -h -o $@ $(ICONS)

icons.cpp: $(ICONS) $(RESWRAP) icons.h
	$(RESWRAP) -i icons.h -o $@ $(ICONS)

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
