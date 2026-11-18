## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = @X_CFLAGS@
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libFOX-2.0.la @GLU_LIBS@ -lm

bin_PROGRAMS = glviewer

glviewer_SOURCES = \
FXGLCone.h FXGLCone.cpp \
FXGLCube.h FXGLCube.cpp \
FXGLCylinder.h FXGLCylinder.cpp \
FXGLLine.h FXGLLine.cpp \
FXGLPoint.h FXGLPoint.cpp \
FXGLShape.h FXGLShape.cpp \
FXGLSphere.h FXGLSphere.cpp \
FXGLTriangleMesh.h FXGLTriangleMesh.cpp \
glviewer.cpp
