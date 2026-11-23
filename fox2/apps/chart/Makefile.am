## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-2.0.la -lm

bin_PROGRAMS = charttest chartdemo

if COND_OSWINDOWS
chartdemo_LDFLAGS = -mwindows
endif()

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
FXData.h \
FXData.cpp \
FXCurve.h \
FXCurve.cpp \
FX2DChart.h \
FX2DChart.cpp \
FX2DPlot.h \
FX2DPlot.cpp \
live/LiveSource.h \
live/LiveSource.cpp \
charttest.cpp

chartdemo_SOURCES = \
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
FXData.h \
FXData.cpp \
FXCurve.h \
FXCurve.cpp \
FX2DChart.h \
FX2DChart.cpp \
FX2DPlot.h \
FX2DPlot.cpp \
CSVReader.h \
CSVReader.cpp \
chartdemo.cpp

EXTRA_DIST = \
marble.bmp
