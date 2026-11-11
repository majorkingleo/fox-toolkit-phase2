## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-1.7.la -lm

noinst_PROGRAMS = hello button

hello_SOURCES = hello.cpp
button_SOURCES = button.cpp
