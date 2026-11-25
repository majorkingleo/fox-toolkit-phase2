## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-2.0.la -lm

bin_PROGRAMS = charttest chartdemo

if COND_OSWINDOWS
charttest_LDFLAGS = -mwindows
chartdemo_LDFLAGS = -mwindows
endif

# reswrap embeds the chart bitmap and light/dark SVG toolbar icons.
BUILT_SOURCES = icons.h icons.cpp
CLEANFILES = icons.h icons.cpp

ICON_SVGS = \
icons/x-axis-label.svg \
icons/y-axis-label.svg \
icons/autofix-y-axis.svg \
icons/split-chart.svg \
icons/reset-zoom.svg \
icons/zoom.svg \
icons/data-series.svg \
icons/crop.svg \
icons/drag.svg \
icons/detach.svg \
icons/dark/x-axis-label.svg \
icons/dark/y-axis-label.svg \
icons/dark/autofix-y-axis.svg \
icons/dark/split-chart.svg \
icons/dark/reset-zoom.svg \
icons/dark/zoom.svg \
icons/dark/data-series.svg \
icons/dark/crop.svg \
icons/dark/drag.svg \
icons/dark/detach.svg
CHART_ICON_INPUTS = $(addprefix $(srcdir)/,$(ICON_SVGS))

ICON_RESOURCES = \
-r chart_x_axis_label $(srcdir)/icons/x-axis-label.svg \
-r chart_y_axis_label $(srcdir)/icons/y-axis-label.svg \
-r chart_autofix_y_axis $(srcdir)/icons/autofix-y-axis.svg \
-r chart_split_chart $(srcdir)/icons/split-chart.svg \
-r chart_reset_zoom $(srcdir)/icons/reset-zoom.svg \
-r chart_zoom $(srcdir)/icons/zoom.svg \
-r chart_data_series $(srcdir)/icons/data-series.svg \
-r chart_crop $(srcdir)/icons/crop.svg \
-r chart_drag $(srcdir)/icons/drag.svg \
-r chart_detach $(srcdir)/icons/detach.svg \
-r chart_dark_x_axis_label $(srcdir)/icons/dark/x-axis-label.svg \
-r chart_dark_y_axis_label $(srcdir)/icons/dark/y-axis-label.svg \
-r chart_dark_autofix_y_axis $(srcdir)/icons/dark/autofix-y-axis.svg \
-r chart_dark_split_chart $(srcdir)/icons/dark/split-chart.svg \
-r chart_dark_reset_zoom $(srcdir)/icons/dark/reset-zoom.svg \
-r chart_dark_zoom $(srcdir)/icons/dark/zoom.svg \
-r chart_dark_data_series $(srcdir)/icons/dark/data-series.svg \
-r chart_dark_crop $(srcdir)/icons/dark/crop.svg \
-r chart_dark_drag $(srcdir)/icons/dark/drag.svg \
-r chart_dark_detach $(srcdir)/icons/dark/detach.svg

icons.h: $(srcdir)/marble.bmp $(CHART_ICON_INPUTS) $(top_builddir)/utils/reswrap$(EXEEXT)
	$(top_builddir)/utils/reswrap$(EXEEXT) -h -o $@ -r marble $(srcdir)/marble.bmp
	$(top_builddir)/utils/reswrap$(EXEEXT) -h -a -oa $@ $(ICON_RESOURCES)

icons.cpp: $(srcdir)/marble.bmp $(CHART_ICON_INPUTS) $(top_builddir)/utils/reswrap$(EXEEXT) icons.h
	$(top_builddir)/utils/reswrap$(EXEEXT) -s -i icons.h -o $@ -r marble $(srcdir)/marble.bmp
	$(top_builddir)/utils/reswrap$(EXEEXT) -s -a -oa $@ $(ICON_RESOURCES)

charttest_SOURCES = \
icons.h \
icons.cpp \
live/LiveSource.h \
live/LiveSource.cpp \
charttest.cpp

chartdemo_SOURCES = \
icons.h \
icons.cpp \
CSVReader.h \
CSVReader.cpp \
ChartDemoWindow.h \
ChartAssignmentDialog.h \
ChartAssignmentDialog.cpp \
ChartDetachedWindow.h \
ChartDetachedWindow.cpp \
ChartDemoUtils.h \
ChartDemoUtils.cpp \
ChartTabItem.h \
ChartTabItem.cpp \
chartdemo.cpp \
chartdemo_main.cpp

EXTRA_DIST = \
marble.bmp \
icons/README.md \
icons/x-axis-label.svg \
icons/y-axis-label.svg \
icons/autofix-y-axis.svg \
icons/split-chart.svg \
icons/reset-zoom.svg \
icons/zoom.svg \
icons/data-series.svg \
icons/crop.svg \
icons/drag.svg \
icons/detach.svg \
icons/dark/x-axis-label.svg \
icons/dark/y-axis-label.svg \
icons/dark/autofix-y-axis.svg \
icons/dark/split-chart.svg \
icons/dark/reset-zoom.svg \
icons/dark/zoom.svg \
icons/dark/data-series.svg \
icons/dark/crop.svg \
icons/dark/drag.svg \
icons/dark/detach.svg
