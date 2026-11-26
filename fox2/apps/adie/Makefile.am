## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-2.0.la -lm
RESWRAP = $(top_builddir)/utils/reswrap$(EXEEXT)

bin_PROGRAMS = adie

if COND_OSWINDOWS
adie_LDFLAGS = -mwindows
endif

bin_SCRIPTS = Adie.stx

man_MANS = adie.1

ICONS = $(wildcard $(srcdir)/*.gif) $(wildcard $(srcdir)/*.bmp)

# icons.h and icons.cpp are generated from the image files by reswrap.
BUILT_SOURCES = icons.h icons.cpp
CLEANFILES = icons.h icons.cpp

icons.h: $(ICONS) $(RESWRAP)
	$(RESWRAP) -h -o $@ $(ICONS)

icons.cpp: $(ICONS) $(RESWRAP) icons.h
	$(RESWRAP) -i icons.h -o $@ $(ICONS)

adie_SOURCES = \
Adie.h \
Adie.cpp \
Commands.cpp \
Commands.h \
FindInFiles.h \
FindInFiles.cpp \
help.h \
help.cpp \
HelpWindow.h \
HelpWindow.cpp \
icons.h \
icons.cpp \
main.cpp \
Modeline.h \
Modeline.cpp \
Preferences.h \
Preferences.cpp \
ReplaceInFiles.h \
ReplaceInFiles.cpp \
ShellCommand.h \
ShellCommand.cpp \
ShellDialog.h \
ShellDialog.cpp \
Syntax.h \
Syntax.cpp \
SyntaxParser.h \
SyntaxParser.cpp \
TextWindow.h \
TextWindow.cpp

EXTRA_DIST = \
$(man_MANS) \
Adie.stx \
LICENSE \
help.txt \
$(wildcard *.gif) \
$(wildcard *.bmp)
