## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-1.7.la -lm

bin_PROGRAMS = adie

bin_SCRIPTS = Adie.stx

man_MANS = adie.1

# icons.h/icons.cpp and help.h/help.cpp are checked in; they are the output of
# reswrap over the images and help.txt, as in the calculator and control
# panel ports.
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
$(wildcard *.svg) \
$(wildcard *.bmp)
