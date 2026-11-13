# Feature detection and compiler settings for FOX.
#
# CMake counterpart of the checks in configure.ac. Included once from the top
# level CMakeLists.txt. The same file is used by fox2/ and fox-1.7.95/.
#
# Variables set for the subdirectories:
#
#   FOX_PUBLIC_DEFINITIONS   definitions for the library and its users
#                            (WIN32, HAVE_xxx_H=1, ...)
#   FOX_PUBLIC_INCLUDE_DIRS  include directories needed by users (X11)
#   FOX_PRIVATE_INCLUDE_DIRS include directories only the library needs
#   FOX_PUBLIC_LIBRARIES     system/GUI libraries (comctl32, X11, GL, ...)
#   FOX_PRIVATE_LIBRARIES    codec libraries (jpeg, zlib, tiff, ...)
#   FOX_GLU_LIBRARIES        extra libraries for programs using GLU
#   FOX_FOXTRACE             true if FXTRACE statements stay compiled in
#
include_guard(GLOBAL)

include(CheckIncludeFileCXX)
include(CheckCXXSymbolExists)

# ---------------------------------------------------------------------------
# Options (names follow the configure switches)
# ---------------------------------------------------------------------------

option(FOX_BUILD_SHARED         "Build libFOX as a shared library" ON)
option(FOX_BUILD_TESTS          "Build the test programs" ON)
option(FOX_FOXTRACE             "Keep FXTRACE statements compiled in (no NDEBUG)" OFF)
option(FOX_PERFORMANCE_LOGGING  "Compile with performance logging" OFF)
set(FOX_NATIVE "" CACHE STRING "Compile for native code: ON, or an architecture like corei7")

option(FOX_WITH_OPENGL "Use OpenGL if found" ON)
option(FOX_WITH_JPEG   "Use libjpeg if found" ON)
option(FOX_WITH_JXL    "Use libjxl if found" ON)
option(FOX_WITH_JP2    "Use openjpeg if found" OFF)
option(FOX_WITH_WEBP   "Use libwebp if found" OFF)
option(FOX_WITH_ZLIB   "Use zlib if found" ON)
option(FOX_WITH_BZ2LIB "Use bzip2 if found" ON)
option(FOX_WITH_LZMA   "Use liblzma if found" ON)
option(FOX_WITH_TIFF   "Use libtiff if found" ON)
option(FOX_WITH_CUPS   "Use CUPS if found" ON)

# X11 backend only
option(FOX_WITH_XSHM    "Use the XShm extension if found" ON)
option(FOX_WITH_XSHAPE  "Use the XShape extension if found" ON)
option(FOX_WITH_XCURSOR "Use Xcursor if found" ON)
option(FOX_WITH_XRENDER "Use XRender if found" ON)
option(FOX_WITH_XRANDR  "Use XRandR if found" ON)
option(FOX_WITH_XFIXES  "Use XFixes if found" ON)
option(FOX_WITH_XINPUT  "Use XInput2 if found" ON)
option(FOX_WITH_XFT     "Use Xft/fontconfig/freetype if found" ON)
option(FOX_WITH_XIM     "Compile with X input method support" OFF)

set(FOX_PUBLIC_DEFINITIONS "")
set(FOX_PUBLIC_INCLUDE_DIRS "")
set(FOX_PRIVATE_INCLUDE_DIRS "")
set(FOX_PUBLIC_LIBRARIES "")
set(FOX_PRIVATE_LIBRARIES "")
set(FOX_GLU_LIBRARIES "")

# ---------------------------------------------------------------------------
# Compiler flags (configure.ac starts with CXXFLAGS="" and sets its own)
# ---------------------------------------------------------------------------

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra -Woverloaded-virtual -Wshadow -Wstrict-overflow -Wstrict-aliasing -Wdouble-promotion -Wno-implicit-fallthrough")
  set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
  if(FOX_FOXTRACE)
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -ffast-math -fstrict-overflow -fstrict-aliasing -finline-functions -fomit-frame-pointer")
  else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -ffast-math -fstrict-overflow -fstrict-aliasing -finline-functions -fomit-frame-pointer")
  endif()
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "-s")

  string(TOUPPER "${FOX_NATIVE}" _fox_native)
  if(_fox_native MATCHES "^(ON|YES|TRUE|1|NATIVE)$")
    string(APPEND CMAKE_CXX_FLAGS " -march=native")
  elseif(FOX_NATIVE AND NOT _fox_native MATCHES "^(OFF|NO|FALSE|0)$")
    string(APPEND CMAKE_CXX_FLAGS " -march=${FOX_NATIVE}")
  endif()
endif()

if(FOX_PERFORMANCE_LOGGING)
  list(APPEND FOX_PUBLIC_DEFINITIONS PERFORMANCE_LOGGING=1)
endif()

# XIM is opt-in: without it NO_XIM is defined
if(NOT FOX_WITH_XIM)
  list(APPEND FOX_PUBLIC_DEFINITIONS NO_XIM)
endif()

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

# Compilers already search their standard include directories; passing them
# again with -I/-isystem breaks #include_next of the C++ library headers.
macro(fox_add_include_dir LISTVAR DIR)
  if(NOT "${DIR}" STREQUAL "" AND NOT "${DIR}" IN_LIST CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
    list(APPEND ${LISTVAR} "${DIR}")
  endif()
endmacro()

# Optional library: needs the header and the library, adds the definition,
# the include directory and the library.
#
#   fox_find_optional(<NAME> HEADER <hdr> LIBS <lib>... [DEFINE <macro>]
#                     [PATH_SUFFIXES <dir>...] [PUBLIC])
macro(fox_find_optional NAME)
  cmake_parse_arguments(_FFO "PUBLIC" "DEFINE" "HEADER;LIBS;PATH_SUFFIXES" ${ARGN})
  if(FOX_WITH_${NAME})
    find_path(FOX_${NAME}_INCLUDE_DIR NAMES ${_FFO_HEADER} PATH_SUFFIXES ${_FFO_PATH_SUFFIXES})
    find_library(FOX_${NAME}_LIBRARY NAMES ${_FFO_LIBS})
    if(FOX_${NAME}_INCLUDE_DIR AND FOX_${NAME}_LIBRARY)
      message(STATUS "FOX: ${NAME} support enabled")
      list(APPEND FOX_PUBLIC_DEFINITIONS ${_FFO_DEFINE}=1)
      if(_FFO_PUBLIC)
        fox_add_include_dir(FOX_PUBLIC_INCLUDE_DIRS "${FOX_${NAME}_INCLUDE_DIR}")
        list(APPEND FOX_PUBLIC_LIBRARIES "${FOX_${NAME}_LIBRARY}")
      else()
        fox_add_include_dir(FOX_PRIVATE_INCLUDE_DIRS "${FOX_${NAME}_INCLUDE_DIR}")
        list(APPEND FOX_PRIVATE_LIBRARIES "${FOX_${NAME}_LIBRARY}")
      endif()
    else()
      message(STATUS "FOX: ${NAME} support disabled (not found)")
    endif()
  endif()
endmacro()

# Read the *.cpp file names out of an Automake file, so the CMake build and
# the Autotools build compile the same sources.
function(fox_read_makefile_sources MAKEFILE OUTVAR)
  file(READ "${MAKEFILE}" _txt)
  string(REGEX MATCHALL "[A-Za-z0-9_]+\\.cpp" _src "${_txt}")
  list(REMOVE_DUPLICATES _src)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${MAKEFILE}")
  set(${OUTVAR} "${_src}" PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# Platform
# ---------------------------------------------------------------------------

find_package(Threads REQUIRED)
list(APPEND FOX_PUBLIC_LIBRARIES Threads::Threads)

# immintrin.h gates the SSE code in xincs.h
check_include_file_cxx(immintrin.h HAVE_IMMINTRIN_H)
if(HAVE_IMMINTRIN_H)
  list(APPEND FOX_PUBLIC_DEFINITIONS HAVE_IMMINTRIN_H=1)
endif()

if(WIN32)
  # WIN32 selects the Win32 backend; users of the library need it too.
  list(APPEND FOX_PUBLIC_DEFINITIONS WIN32)
  list(APPEND FOX_PUBLIC_LIBRARIES comctl32 wsock32 winspool mpr gdi32 imm32 msimg32)
  set(_fox_gl_libs opengl32)
  set(_fox_glu_libs glu32)
else()
  set(_fox_gl_libs GL)
  set(_fox_glu_libs GLU)

  # The Win32 branches of the sources never look at these macros, so the
  # POSIX probes are skipped on Windows (this is what makes configure slow).
  foreach(_hdr unistd.h sys/wait.h dirent.h sys/select.h sys/epoll.h sys/timerfd.h
               sys/ipc.h sys/shm.h sys/mman.h sys/pstat.h sys/mount.h semaphore.h
               dlfcn.h sys/statvfs.h sys/inotify.h sys/resource.h sys/sysctl.h
               sys/utsname.h)
    string(TOUPPER "${_hdr}" _var)
    string(REGEX REPLACE "[/.]" "_" _var "${_var}")
    check_include_file_cxx(${_hdr} HAVE_${_var})
    if(HAVE_${_var})
      list(APPEND FOX_PUBLIC_DEFINITIONS HAVE_${_var}=1)
    endif()
  endforeach()

  # <symbol>:<header> pairs
  foreach(_pair localtime_r:time.h gmtime_r:time.h getpwuid_r:pwd.h getgrgid_r:grp.h
                getpwnam_r:pwd.h pthread_setaffinity_np:pthread.h
                pthread_getname_np:pthread.h pthread_setname_np:pthread.h
                sched_getcpu:sched.h epoll_create1:sys/epoll.h
                timerfd_create:sys/timerfd.h uname:sys/utsname.h pipe2:unistd.h
                statvfs:sys/statvfs.h getrlimit:sys/resource.h daemon:unistd.h
                inotify_init1:sys/inotify.h)
    string(REPLACE ":" ";" _pair "${_pair}")
    list(GET _pair 0 _sym)
    list(GET _pair 1 _hdr)
    string(TOUPPER "${_sym}" _var)
    check_cxx_symbol_exists(${_sym} ${_hdr} HAVE_${_var})
    if(HAVE_${_var})
      list(APPEND FOX_PUBLIC_DEFINITIONS HAVE_${_var}=1)
    endif()
  endforeach()

  # clock_gettime and dlopen live in separate libraries on older systems
  find_library(FOX_RT_LIBRARY rt)
  if(FOX_RT_LIBRARY)
    list(APPEND FOX_PUBLIC_LIBRARIES "${FOX_RT_LIBRARY}")
  endif()
  if(CMAKE_DL_LIBS)
    list(APPEND FOX_PUBLIC_LIBRARIES ${CMAKE_DL_LIBS})
  endif()

  # X11 backend
  find_path(FOX_X11_INCLUDE_DIR NAMES X11/Xlib.h)
  find_library(FOX_X11_LIBRARY NAMES X11)
  find_library(FOX_XEXT_LIBRARY NAMES Xext)
  if(NOT FOX_X11_INCLUDE_DIR OR NOT FOX_X11_LIBRARY OR NOT FOX_XEXT_LIBRARY)
    message(FATAL_ERROR "X11 development files (X11/Xlib.h, libX11, libXext) not found")
  endif()
  fox_add_include_dir(FOX_PUBLIC_INCLUDE_DIRS "${FOX_X11_INCLUDE_DIR}")
  list(APPEND FOX_PUBLIC_LIBRARIES "${FOX_X11_LIBRARY}" "${FOX_XEXT_LIBRARY}")

  fox_find_optional(XSHM    HEADER X11/extensions/XShm.h    LIBS Xext    DEFINE HAVE_XSHM_H)
  fox_find_optional(XSHAPE  HEADER X11/extensions/shape.h   LIBS Xext    DEFINE HAVE_XSHAPE_H)
  fox_find_optional(XCURSOR HEADER X11/Xcursor/Xcursor.h    LIBS Xcursor DEFINE HAVE_XCURSOR_H PUBLIC)
  fox_find_optional(XRENDER HEADER X11/extensions/Xrender.h LIBS Xrender DEFINE HAVE_XRENDER_H PUBLIC)
  fox_find_optional(XRANDR  HEADER X11/extensions/Xrandr.h  LIBS Xrandr  DEFINE HAVE_XRANDR_H PUBLIC)
  fox_find_optional(XFIXES  HEADER X11/extensions/Xfixes.h  LIBS Xfixes  DEFINE HAVE_XFIXES_H PUBLIC)
  fox_find_optional(XINPUT  HEADER X11/extensions/XInput2.h LIBS Xi      DEFINE HAVE_XINPUT2_H PUBLIC)

  if(FOX_WITH_XFT)
    find_path(FOX_XFT_INCLUDE_DIR NAMES X11/Xft/Xft.h)
    find_path(FOX_FREETYPE_INCLUDE_DIR NAMES ft2build.h PATH_SUFFIXES freetype2)
    find_path(FOX_FONTCONFIG_INCLUDE_DIR NAMES fontconfig/fontconfig.h)
    find_library(FOX_XFT_LIBRARY NAMES Xft)
    find_library(FOX_FREETYPE_LIBRARY NAMES freetype)
    find_library(FOX_FONTCONFIG_LIBRARY NAMES fontconfig)
    if(FOX_XFT_INCLUDE_DIR AND FOX_FREETYPE_INCLUDE_DIR AND FOX_FONTCONFIG_INCLUDE_DIR
       AND FOX_XFT_LIBRARY AND FOX_FREETYPE_LIBRARY AND FOX_FONTCONFIG_LIBRARY)
      message(STATUS "FOX: Xft support enabled")
      list(APPEND FOX_PUBLIC_DEFINITIONS HAVE_XFT_H=1)
      fox_add_include_dir(FOX_PUBLIC_INCLUDE_DIRS "${FOX_XFT_INCLUDE_DIR}")
      fox_add_include_dir(FOX_PUBLIC_INCLUDE_DIRS "${FOX_FREETYPE_INCLUDE_DIR}")
      fox_add_include_dir(FOX_PUBLIC_INCLUDE_DIRS "${FOX_FONTCONFIG_INCLUDE_DIR}")
      list(APPEND FOX_PUBLIC_LIBRARIES "${FOX_XFT_LIBRARY}" "${FOX_FONTCONFIG_LIBRARY}" "${FOX_FREETYPE_LIBRARY}")
    else()
      message(STATUS "FOX: Xft support disabled (not found)")
    endif()
  endif()
endif()

# ---------------------------------------------------------------------------
# Optional libraries
# ---------------------------------------------------------------------------

fox_find_optional(JPEG   HEADER jpeglib.h       LIBS jpeg DEFINE HAVE_JPEG_H)
fox_find_optional(JXL    HEADER jxl/decode.h    LIBS jxl DEFINE HAVE_JXL_H)
fox_find_optional(JP2    HEADER openjpeg.h      LIBS openjp2 openjpeg DEFINE HAVE_JP2_H
                         PATH_SUFFIXES openjpeg-2.5 openjpeg-2.4 openjpeg-2.3)
fox_find_optional(WEBP   HEADER webp/decode.h   LIBS webp DEFINE HAVE_WEBP_H)
fox_find_optional(ZLIB   HEADER zlib.h          LIBS z DEFINE HAVE_ZLIB_H)
fox_find_optional(BZ2LIB HEADER bzlib.h         LIBS bz2 DEFINE HAVE_BZ2LIB_H)
fox_find_optional(LZMA   HEADER lzma.h          LIBS lzma DEFINE HAVE_LZMA_H)
fox_find_optional(TIFF   HEADER tiff.h          LIBS tiff DEFINE HAVE_TIFF_H)
fox_find_optional(CUPS   HEADER cups/cups.h     LIBS cups DEFINE HAVE_CUPS_H)

# ---------------------------------------------------------------------------
# OpenGL
# ---------------------------------------------------------------------------

if(FOX_WITH_OPENGL)
  find_path(FOX_GL_INCLUDE_DIR NAMES GL/gl.h)
  find_path(FOX_GLU_INCLUDE_DIR NAMES GL/glu.h)
  find_library(FOX_GL_LIBRARY NAMES ${_fox_gl_libs})
  find_library(FOX_GLU_LIBRARY NAMES ${_fox_glu_libs})
  if(FOX_GL_INCLUDE_DIR AND FOX_GL_LIBRARY)
    message(STATUS "FOX: OpenGL support enabled")
    list(APPEND FOX_PUBLIC_DEFINITIONS HAVE_GL_H=1)
    fox_add_include_dir(FOX_PUBLIC_INCLUDE_DIRS "${FOX_GL_INCLUDE_DIR}")
    list(APPEND FOX_PUBLIC_LIBRARIES "${FOX_GL_LIBRARY}")
    if(FOX_GLU_INCLUDE_DIR)
      list(APPEND FOX_PUBLIC_DEFINITIONS HAVE_GLU_H=1)
      fox_add_include_dir(FOX_PUBLIC_INCLUDE_DIRS "${FOX_GLU_INCLUDE_DIR}")
    endif()
    if(FOX_GLU_LIBRARY)
      set(FOX_GLU_LIBRARIES "${FOX_GLU_LIBRARY}")
    endif()
    if(NOT WIN32)
      find_path(FOX_GLX_INCLUDE_DIR NAMES GL/glx.h)
      if(FOX_GLX_INCLUDE_DIR)
        list(APPEND FOX_PUBLIC_DEFINITIONS HAVE_GLX_H=1)
      endif()
    endif()
  else()
    message(STATUS "FOX: OpenGL support disabled (not found)")
  endif()
endif()

list(REMOVE_DUPLICATES FOX_PUBLIC_DEFINITIONS)
