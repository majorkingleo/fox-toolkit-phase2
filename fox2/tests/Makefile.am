## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-1.7.la -lm

noinst_PROGRAMS = hello button dpi

hello_SOURCES = hello.cpp
button_SOURCES = button.cpp
dpi_SOURCES = dpi.cpp
raster_SOURCES = raster.cpp
noinst_PROGRAMS += raster

noinst_PROGRAMS += codecs dictest expression format half match math memmap minheritance parallel process rex scan thread timefmt unicode units variant xml

# The rasterizer test also uses the private fxraster.h in ../lib
AM_CPPFLAGS = -I$(top_srcdir)/lib

codecs_SOURCES = codecs.cpp
dictest_SOURCES = dictest.cpp
expression_SOURCES = expression.cpp
format_SOURCES = format.cpp
half_SOURCES = half.cpp
match_SOURCES = match.cpp
math_SOURCES = math.cpp
memmap_SOURCES = memmap.cpp
minheritance_SOURCES = minheritance.cpp
parallel_SOURCES = parallel.cpp
process_SOURCES = process.cpp
rex_SOURCES = rex.cpp
scan_SOURCES = scan.cpp
thread_SOURCES = thread.cpp
timefmt_SOURCES = timefmt.cpp
unicode_SOURCES = unicode.cpp
units_SOURCES = units.cpp
variant_SOURCES = variant.cpp
xml_SOURCES = xml.cpp
