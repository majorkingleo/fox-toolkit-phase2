## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-2.0.la -lm

bin_PROGRAMS = charttest

# icons.h and icons.cpp are checked in; they are the output of reswrap over
# marble.bmp, as in the calculator and control panel ports.
charttest_SOURCES = \
icons.h \
icons.cpp \
chartdefs.h \
chart.h \
chartutils.h \
chartutils.cpp \
FXAxis.h \
FXAxis.cpp \
FXChart.h \
FXChart.cpp \
FXCurve.h \
FXCurve.cpp \
FX2DChart.h \
FX2DChart.cpp \
FX2DPlot.h \
FX2DPlot.cpp \
charttest.cpp

EXTRA_DIST = \
marble.bmp
