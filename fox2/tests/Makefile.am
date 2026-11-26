## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-2.0.la -lm

noinst_PROGRAMS = hello button dpi popup click menus tooltip clipboard dnd icon bmpicon textspin combo colordlg colordemo fontdlg rulerdemo gaugedemo idle

if COND_OSWINDOWS
hello_LDFLAGS = -mwindows
button_LDFLAGS = -mwindows
dpi_LDFLAGS = -mwindows
popup_LDFLAGS = -mwindows
click_LDFLAGS = -mwindows
menus_LDFLAGS = -mwindows
tooltip_LDFLAGS = -mwindows
clipboard_LDFLAGS = -mwindows
dnd_LDFLAGS = -mwindows
icon_LDFLAGS = -mwindows
bmpicon_LDFLAGS = -mwindows
textspin_LDFLAGS = -mwindows
combo_LDFLAGS = -mwindows
colordlg_LDFLAGS = -mwindows
colordemo_LDFLAGS = -mwindows
fontdlg_LDFLAGS = -mwindows
rulerdemo_LDFLAGS = -mwindows
gaugedemo_LDFLAGS = -mwindows
idle_LDFLAGS = -mwindows
endif

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
gaugedemo_SOURCES = gaugedemo.cpp
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

noinst_PROGRAMS += codecs consolemodel dictest expression format glyphs half match math memmap minheritance parallel process region rex scan terminalmodel thread timefmt unicode units variant winmap xml svg

# The rasterizer test also uses the private fxraster.h in ../lib
AM_CPPFLAGS = -I$(top_srcdir)/lib

codecs_SOURCES = codecs.cpp
consolemodel_SOURCES = consolemodel.cpp
terminalmodel_SOURCES = terminalmodel.cpp
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
