## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-1.7.la -lm

noinst_PROGRAMS = hello button dpi popup click menus tooltip clipboard dnd icon bmpicon textspin combo colordlg colordemo fontdlg rulerdemo idle

# Console tests need no display
noinst_PROGRAMS += desktop

hello_SOURCES = hello.cpp
desktop_SOURCES = desktop.cpp
icon_SOURCES = icon.cpp
idle_SOURCES = idle.cpp
bmpicon_SOURCES = bmpicon.cpp
textspin_SOURCES = textspin.cpp
combo_SOURCES = combo.cpp
colordlg_SOURCES = colordlg.cpp
colordemo_SOURCES = colordemo.cpp
rulerdemo_SOURCES = rulerdemo.cpp
fontdlg_SOURCES = fontdlg.cpp
button_SOURCES = button.cpp
dpi_SOURCES = dpi.cpp
popup_SOURCES = popup.cpp
click_SOURCES = click.cpp
menus_SOURCES = menus.cpp
tooltip_SOURCES = tooltip.cpp
clipboard_SOURCES = clipboard.cpp
dnd_SOURCES = dnd.cpp
raster_SOURCES = raster.cpp
noinst_PROGRAMS += raster

noinst_PROGRAMS += codecs dictest expression format glyphs half match math memmap minheritance parallel process region rex scan thread timefmt unicode units variant winmap xml svg

# The rasterizer test also uses the private fxraster.h in ../lib
AM_CPPFLAGS = -I$(top_srcdir)/lib

codecs_SOURCES = codecs.cpp
dictest_SOURCES = dictest.cpp
expression_SOURCES = expression.cpp
format_SOURCES = format.cpp
glyphs_SOURCES = glyphs.cpp
half_SOURCES = half.cpp
match_SOURCES = match.cpp
math_SOURCES = math.cpp
memmap_SOURCES = memmap.cpp
minheritance_SOURCES = minheritance.cpp
parallel_SOURCES = parallel.cpp
process_SOURCES = process.cpp
region_SOURCES = region.cpp
rex_SOURCES = rex.cpp
scan_SOURCES = scan.cpp
thread_SOURCES = thread.cpp
timefmt_SOURCES = timefmt.cpp
unicode_SOURCES = unicode.cpp
units_SOURCES = units.cpp
variant_SOURCES = variant.cpp
winmap_SOURCES = winmap.cpp
xml_SOURCES = xml.cpp
svg_SOURCES = svg.cpp
