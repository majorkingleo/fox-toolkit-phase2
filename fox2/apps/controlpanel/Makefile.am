## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-2.0.la -lm
RESWRAP = $(top_builddir)/utils/reswrap$(EXEEXT)

bin_PROGRAMS = ControlPanel

if COND_OSWINDOWS
ControlPanel_LDFLAGS = -mwindows
endif

man_MANS = ControlPanel.1

ICONS = \
controlpanel_gif.gif \
colors_gif.gif \
filebinding_gif.gif \
settings_gif.gif

ICON_PATHS = $(addprefix $(srcdir)/,$(ICONS))

# icons.h and icons.cpp are generated from the image files by reswrap.
BUILT_SOURCES = icons.h icons.cpp
CLEANFILES = icons.h icons.cpp

icons.h: $(ICON_PATHS) $(RESWRAP)
	$(RESWRAP) -h -o $@ $(ICON_PATHS)

icons.cpp: $(ICON_PATHS) $(RESWRAP) icons.h
	$(RESWRAP) -i icons.h -o $@ $(ICON_PATHS)

ControlPanel_SOURCES = \
icons.h \
icons.cpp \
ControlPanel.h \
ControlPanel.cpp

EXTRA_DIST = \
$(man_MANS) \
LICENSE \
$(ICONS)
