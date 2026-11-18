## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-2.0.la -lm

bin_PROGRAMS = calculator

man_MANS = calculator.1

IMAGES = \
colors.gif \
information.gif \
bigcalc.gif \
constmem.bmp \
question.gif \
tinycalc.gif

# icons.h and icons.cpp are checked in; the reswrap rule of the original
# Makefile.am comes back when fox2/utils/reswrap is ported.
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
