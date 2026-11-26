## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-2.0.la -lm
RESWRAP = $(top_builddir)/utils/reswrap$(EXEEXT)

bin_PROGRAMS = calculator

if COND_OSWINDOWS
calculator_LDFLAGS = -mwindows
endif

man_MANS = calculator.1

IMAGES = \
colors.gif \
information.gif \
bigcalc.gif \
constmem.bmp \
question.gif \
tinycalc.gif

ICON_PATHS = $(addprefix $(srcdir)/,$(IMAGES))

# icons.h and icons.cpp are generated from the image files by reswrap.
BUILT_SOURCES = icons.h icons.cpp
CLEANFILES = icons.h icons.cpp

icons.h: $(ICON_PATHS) $(RESWRAP)
	$(RESWRAP) -h -o $@ $(ICON_PATHS)

icons.cpp: $(ICON_PATHS) $(RESWRAP) icons.h
	$(RESWRAP) -i icons.h -o $@ $(ICON_PATHS)

calculator_SOURCES = \
icons.h \
icons.cpp \
Calculator.h \
Calculator.cpp \
HelpWindow.h \
HelpWindow.cpp \
Preferences.h \
Preferences.cpp \
help.cpp \
main.cpp

EXTRA_DIST = \
$(man_MANS) \
LICENSE \
$(IMAGES)
