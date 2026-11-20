## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-2.0.la -lm

bin_PROGRAMS = stopwatch

if COND_OSWINDOWS
stopwatch_LDFLAGS = -mwindows
endif

stopwatch_SOURCES = \
ClockView.h \
ClockView.cpp \
History.h \
History.cpp \
HistoryWindow.h \
HistoryWindow.cpp \
StopWatch.h \
StopWatch.cpp \
main.cpp
