## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-2.0.la -lm

bin_PROGRAMS = foxterm
man_MANS = foxterm.1

if COND_OSWINDOWS
foxterm_LDFLAGS = -mwindows
endif

foxterm_SOURCES = \
Profiles.h \
Profiles.cpp \
TerminalTabBar.h \
TerminalTabBar.cpp \
TerminalWindow.h \
TerminalWindow.cpp \
main.cpp

EXTRA_DIST = foxterm.1
