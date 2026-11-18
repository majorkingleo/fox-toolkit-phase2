#!/usr/bin/env bash
#
# Cross-compile fox2 for Windows with MinGW-w64 on Linux (CachyOS/Arch).
#
# This is the fast compile check for the Win32 backend on the build host;
# the resulting DLL and test programs can be smoke-tested with Wine.  The
# build is out-of-tree in tmp/build-fox2-cross, or in
# tmp/build-fox2-cross-cmake with --cmake.  The MinGW runtime DLLs are copied
# next to the programs, so the folder runs without WINEPATH.
#
#   ./scripts/build-fox2-mingw-cross.sh
#
# Options:
#   --opengl   enable OpenGL (needs mingw-w64-mesa)
#   --debug    compile with -g -DDEBUG
#   --release  compile with -O2 -DNDEBUG
#   --jobs=N   parallel jobs (default: all cores)
#   --clean    delete the build directory first
#   --fast     skip autoreconf/configure when their inputs did not change
#   --trace    keep FXTRACE statements compiled in (no NDEBUG)
#   --run      run hello, button and dpi under Wine after building
#   --cmake    use CMake + Ninja instead of Autotools: no autoreconf and no
#              configure run, and every test program is built into bin/
#   --install[=DIR] install after building (default prefix: /usr/local)
#   -h|--help  show this help
#
set -euo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
SRC="$ROOT/fox2"
HOST=x86_64-w64-mingw32
SYSROOT=${MINGW_SYSROOT:-/usr/x86_64-w64-mingw32}

jobs=$(nproc 2>/dev/null || echo 4)
with_opengl=no
build_type=
clean=no
fast=no
trace=no
run_test=no
use_cmake=no
run_seconds=5
install_prefix=

usage(){
  sed -n '2,26p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --opengl)   with_opengl=yes ;;
    --debug)    build_type=debug ;;
    --release)  build_type=release ;;
    --clean)    clean=yes ;;
    --fast)     fast=yes ;;
    --trace)    trace=yes ;;
    --run)      run_test=yes ;;
    --cmake)    use_cmake=yes ;;
    --install)
      if [ $# -ge 2 ] && [ "${2#-}" = "$2" ]; then
        install_prefix=$2; shift
      else
        install_prefix=/usr/local
        fi ;;
    --install=*) install_prefix=${1#*=}; [ -n "$install_prefix" ] || install_prefix=/usr/local ;;
    --jobs)     shift; jobs=$1 ;;
    --jobs=*)   jobs=${1#*=} ;;
    --run-seconds) shift; run_seconds=$1 ;;
    --run-seconds=*) run_seconds=${1#*=} ;;
    -h|--help)  usage; exit 0 ;;
    *) echo "error: unknown option '$1'" >&2; usage >&2; exit 2 ;;
    esac
  shift
  done

if [ -n "$install_prefix" ]; then
  case "$install_prefix" in
    /*|[A-Za-z]:/*) ;;
    *) echo "error: --install needs an absolute prefix: '$install_prefix'" >&2; exit 2 ;;
  esac
  fi
cmake_install_prefix=$install_prefix
if [ -n "$install_prefix" ] && [ -n "${MSYSTEM-}" ] && command -v cygpath >/dev/null 2>&1; then
  cmake_install_prefix=$(cygpath -m "$install_prefix")
  fi

if [ -n "${MSYSTEM-}" ]; then
  echo "error: this is the cross build; in MSYS2 use ./scripts/build-fox2-mingw.sh" >&2
  exit 1
  fi

command -v "$HOST-g++" >/dev/null 2>&1 || {
  echo "error: $HOST-g++ not found." >&2
  echo "       run ./scripts/install-cachyos-mingw.sh first." >&2
  exit 1
  }
command -v "$HOST-objdump" >/dev/null 2>&1 || {
  echo "error: $HOST-objdump not found; the MinGW binutils are needed to find" >&2
  echo "       the runtime DLLs of the build." >&2
  exit 1
  }
if [ "$use_cmake" = yes ]; then
  for tool in cmake ninja "$HOST-windres"; do
    command -v "$tool" >/dev/null 2>&1 || {
      echo "error: $tool not found." >&2
      exit 1
      }
    done
else
  command -v make >/dev/null 2>&1 || {
    echo "error: make not found." >&2
    exit 1
    }
  fi
[ -f "$SRC/configure.ac" ] || {
  echo "error: $SRC/configure.ac not found." >&2
  exit 1
  }
if [ "$run_test" = yes ] && ! command -v wine >/dev/null 2>&1; then
  echo "error: --run needs wine; ./scripts/install-cachyos-mingw.sh installs it." >&2
  exit 1
  fi

# The two build systems keep their output apart, so either can be used without
# disturbing the other
if [ "$use_cmake" = yes ]; then
  BUILD="$ROOT/tmp/build-fox2-cross-cmake"
else
  BUILD="$ROOT/tmp/build-fox2-cross"
  fi
LOG="$BUILD/build.log"

if [ "$clean" = yes ]; then
  echo "==> removing $BUILD"
  rm -rf -- "$BUILD"
  fi
mkdir -p -- "$BUILD"

# WIN32 selects the Win32 backend; lib/Makefile.am adds it for the library
# only, so expose it through CPPFLAGS to the tests and tools as well.  CMake
# puts the define on the library and its users itself.
export CPPFLAGS="-DWIN32 ${CPPFLAGS-}"

opts=("--host=$HOST" --without-x)
if [ "$with_opengl" = yes ]; then
  opts+=(--with-opengl=yes)
else
  opts+=(--with-opengl=no)
  fi
opts+=(--disable-cups)
if [ -n "$install_prefix" ]; then
  opts+=(--prefix="$install_prefix")
  fi
case "$build_type" in
  debug)   opts+=(--enable-debug) ;;
  release) opts+=(--enable-release) ;;
esac
if [ "$trace" = yes ]; then
  if [ "$build_type" = release ]; then
    echo "error: --trace cannot be combined with --release" >&2
    exit 2
    fi
  opts+=(--enable-foxtrace)
  fi

# CMake has no configure script; the target platform and the compilers are on
# the command line, and the options follow the configure switches
copts=(-S "$SRC" -B "$BUILD" -G Ninja
       -DCMAKE_SYSTEM_NAME=Windows
       -DCMAKE_CXX_COMPILER=$HOST-g++
       -DCMAKE_C_COMPILER=$HOST-gcc
       -DCMAKE_RC_COMPILER=$HOST-windres
       -DFOX_PLATFORM=win32
       -DFOX_WITH_CUPS=OFF)
if [ "$with_opengl" = yes ]; then
  copts+=(-DFOX_WITH_OPENGL=ON)
else
  copts+=(-DFOX_WITH_OPENGL=OFF)
  fi
case "$build_type" in
  debug)   copts+=(-DCMAKE_BUILD_TYPE=Debug) ;;
  release) copts+=(-DCMAKE_BUILD_TYPE=Release) ;;
esac
if [ "$trace" = yes ]; then
  copts+=(-DFOX_FOXTRACE=ON)
  fi

: >"$LOG"

run(){
  echo "*** $*" >>"$LOG"
  "$@" 2>&1 | tee -a "$LOG"
  }

# The programs need the MinGW runtime DLLs (libstdc++-6, libgcc_s_seh-1,
# libwinpthread-1), which live in the sysroot and not in the build tree.  Copy
# every DLL that objdump reports and the sysroot provides next to the programs,
# so the folder runs under Wine without WINEPATH and can be copied to a Windows
# machine as it is.  Three passes catch the DLLs that a copied DLL needs.
copy_runtime_dlls(){
  local dir f name pass n
  [ -d "$SYSROOT/bin" ] || return 0
  for pass in 1 2 3; do
    n=0
    for dir in "$@"; do
      [ -d "$dir" ] || continue
      for f in "$dir"/*.exe "$dir"/*.dll; do
        [ -f "$f" ] || continue
        while read -r name; do
          [ -n "$name" ] || continue
          [ -f "$dir/$name" ] && continue
          [ -f "$SYSROOT/bin/$name" ] || continue
          cp -- "$SYSROOT/bin/$name" "$dir/" || return 1
          echo "==> copied $name"
          n=$((n+1))
        done < <("$HOST-objdump" -p "$f" 2>/dev/null | sed -n 's/^[[:space:]]*DLL Name: //p')
      done
    done
    [ "$n" -eq 0 ] && break
  done
  return 0
  }

# The generated configure is stale when its autotools inputs are newer
needs_autoreconf(){
  [ -x "$SRC/configure" ] || return 0
  [ "$SRC/configure.ac" -nt "$SRC/configure" ] && return 0
  [ "$SRC/aclocal.m4" -nt "$SRC/configure" ] && return 0
  [ -n "$(find "$SRC" -name 'Makefile.am' -newer "$SRC/configure" -print -quit)" ] && return 0
  return 1
  }

# The build directory is stale when configure or any Makefile.in is newer
needs_configure(){
  [ -f "$BUILD/Makefile" ] || return 0
  [ -f "$BUILD/.configure-options" ] || return 0
  [ "$(cat "$BUILD/.configure-options")" != "${opts[*]}" ] && return 0
  [ "$SRC/configure" -nt "$BUILD/Makefile" ] && return 0
  [ -n "$(find "$SRC" -name 'Makefile.in' -newer "$BUILD/Makefile" -print -quit)" ] && return 0
  return 1
  }

if [ "$use_cmake" = no ]; then

if [ "$fast" = yes ] && ! needs_autoreconf; then
  echo "==> autoreconf skipped (configure is up to date)"
else
  echo "==> autoreconf"
  ( cd -- "$SRC" && autoreconf -fi ) >"$BUILD/autoreconf.log" 2>&1 || {
    echo "error: autoreconf failed, see $BUILD/autoreconf.log" >&2
    tail -n 20 "$BUILD/autoreconf.log" >&2
    exit 1
    }
  fi

cd -- "$BUILD"

if [ "$fast" = yes ] && ! needs_configure; then
  echo "==> configure skipped (build directory is up to date)"
  reconfigured=no
else
  echo "==> configure: $SRC/configure ${opts[*]}"
  run "$SRC/configure" "${opts[@]}"
  printf '%s\n' "${opts[*]}" >"$BUILD/.configure-options"
  reconfigured=yes
  fi

# Compiler and linker flags end up inside the object files and the DLL, and
# make does not rebuild them when the flags change; start over in that case.
if [ "$reconfigured" = yes ]; then
  echo "==> cleaning objects (configure was rerun)"
  run make -C lib clean
  run make -C tests clean
  fi

# The Win32 backend must be selected.  --without-x disables X detection, but
# AC_PATH_XTRA still probes for X libraries later on; with a target sysroot
# that has no X11 headers and libraries those probes must fail.
if ! grep -q '^X_BASE_LIBS = .*-lcomctl32' "$BUILD/lib/Makefile" ||
   grep -q '^X_BASE_LIBS = .*-lX' "$BUILD/lib/Makefile"; then
  echo "error: configure did not select the Win32 platform libraries." >&2
  echo "       X_BASE_LIBS in $BUILD/lib/Makefile is:" >&2
  grep '^X_BASE_LIBS =' "$BUILD/lib/Makefile" >&2 || true
  exit 1
  fi

echo "==> build library"
run make -j"$jobs" -C lib

echo "==> build tests"
run make -j"$jobs" -C tests hello.exe button.exe dpi.exe desktop.exe icon.exe raster.exe region.exe glyphs.exe winmap.exe

if [ -f apps/Makefile ]; then
  echo "==> build applications"
  run make -j"$jobs" -C apps
  fi

echo "==> build utilities"
run make -j"$jobs" -C utils

echo "==> runtime DLLs"
copy_runtime_dlls "$BUILD/lib/.libs" "$BUILD/tests/.libs" "$BUILD/apps/calculator/.libs" "$BUILD/utils/.libs" "$BUILD/utils"

echo
echo "==> artifacts"
ls -l lib/.libs/*.dll lib/.libs/*.dll.a 2>/dev/null || true
ls -l tests/.libs/hello.exe tests/.libs/button.exe tests/.libs/dpi.exe tests/.libs/raster.exe tests/.libs/region.exe tests/.libs/glyphs.exe tests/.libs/winmap.exe 2>/dev/null || true
ls -l apps/calculator/.libs/calculator.exe 2>/dev/null || true
ls -l utils/.libs/reswrap.exe utils/reswrap.exe 2>/dev/null || true

else

# The build directory is stale when the options or the CMake inputs changed
needs_cmake_configure(){
  [ -f "$BUILD/CMakeCache.txt" ] || return 0
  [ -f "$BUILD/.cmake-options" ] || return 0
  [ "$(cat "$BUILD/.cmake-options")" != "${copts[*]}" ] && return 0
  [ -n "$(find "$SRC" -name 'CMakeLists.txt' -o -name '*.cmake' -newer "$BUILD/CMakeCache.txt" -print -quit)" ] && return 0
  return 1
  }

cd -- "$BUILD"

if [ "$fast" = yes ] && ! needs_cmake_configure; then
  echo "==> cmake configure skipped (build directory is up to date)"
else
  echo "==> cmake configure: FOX_PLATFORM=win32 $HOST-g++"
  run cmake "${copts[@]}"
  printf '%s\n' "${copts[*]}" >"$BUILD/.cmake-options"
  fi

echo "==> build library, tests, applications and tools"
run cmake --build "$BUILD" -j "$jobs"

echo "==> runtime DLLs"
copy_runtime_dlls "$BUILD/bin"

echo
echo "==> artifacts"
ls -l "$BUILD"/bin/*.dll "$BUILD"/lib/*.dll.a 2>/dev/null || true
ls -l "$BUILD"/bin/hello.exe "$BUILD"/bin/button.exe "$BUILD"/bin/dpi.exe "$BUILD"/bin/popup.exe 2>/dev/null || true
ls -l "$BUILD"/bin/calculator.exe "$BUILD"/bin/reswrap.exe 2>/dev/null || true

fi

if [ "$run_test" = yes ]; then
  # A dedicated prefix keeps the user's Wine setup untouched; the first run
  # creates it, which takes a while.
  if [ -z "${WINEPREFIX-}" ]; then
    WINEPREFIX="$ROOT/tmp/wineprefix"
    fi
  export WINEPREFIX
  # The import libraries link against the MinGW runtime DLLs, which wine has
  # to find next to our own DLL.
  export WINEDEBUG="${WINEDEBUG:--all}"
  export WINEDLLOVERRIDES="${WINEDLLOVERRIDES:-mscoree,mshtml=}"
  export WINEPATH="$BUILD/bin;$BUILD/lib/.libs;$SYSROOT/bin"
  if [ ! -d "$WINEPREFIX" ]; then
    echo
    echo "==> initializing the Wine prefix in $WINEPREFIX"
    timeout 180 wineboot --init >/dev/null 2>&1 || true
    fi
  # Autotools writes the programs to tests/.libs and apps/*/.libs, CMake puts
  # them all in bin/ next to the DLL
  find_exe(){
    local prog=$1 p
    for p in "$BUILD/bin/$prog.exe" "$BUILD/tests/.libs/$prog.exe" "$BUILD/tests/$prog.exe" \
             "$BUILD/apps/$prog/.libs/$prog.exe" "$BUILD/apps/$prog/$prog.exe" \
             "$BUILD/utils/.libs/$prog.exe" "$BUILD/utils/$prog.exe"; do
      if [ -f "$p" ]; then
        printf '%s\n' "$p"
        return 0
        fi
      done
    return 1
    }
  for prog in hello button dpi; do
    exe=$(find_exe "$prog") || true
    echo
    echo "==> running $prog under Wine for ${run_seconds} seconds"
    rc=0
    timeout "$run_seconds" wine "$exe" >"$BUILD/$prog.wine.log" 2>&1 || rc=$?
    tail -n 20 "$BUILD/$prog.wine.log"
    if [ "$rc" -eq 124 ]; then
      echo "$prog stayed in its event loop (expected)"
    elif [ "$rc" -eq 0 ]; then
      echo "note: $prog exited on its own"
    else
      echo "warning: $prog exited with status $rc" >&2
      echo "         retrying with Wine diagnostics" >&2
      rc=0
      timeout "$run_seconds" env WINEDEBUG= wine "$exe" >>"$BUILD/$prog.wine.log" 2>&1 || rc=$?
      tail -n 20 "$BUILD/$prog.wine.log" >&2
      if [ "$rc" -eq 124 ]; then
        echo "$prog reached its event loop with diagnostics; runtime DLLs were missing before"
      fi
      fi
    done
  echo
  echo "==> rasterizer scene under Wine (headless)"
  exe=$(find_exe raster) || true
  rc=0
  timeout "$run_seconds" wine "$exe" --raster-only || rc=$?
  [ "$rc" -eq 0 ] || echo "warning: raster exited with status $rc" >&2
  echo
  echo "==> rectangle region test under Wine"
  exe=$(find_exe region) || true
  rc=0
  timeout "$run_seconds" wine "$exe" || rc=$?
  [ "$rc" -eq 0 ] || echo "warning: region exited with status $rc" >&2
  echo
  echo "==> glyph cache test under Wine"
  exe=$(find_exe glyphs) || true
  rc=0
  timeout "$run_seconds" wine "$exe" || rc=$?
  [ "$rc" -eq 0 ] || echo "warning: glyphs exited with status $rc" >&2
  echo
  echo "==> virtual window map test under Wine"
  exe=$(find_exe winmap) || true
  rc=0
  timeout "$run_seconds" wine "$exe" || rc=$?
  [ "$rc" -eq 0 ] || echo "warning: winmap exited with status $rc" >&2

  echo
  echo "==> reswrap under Wine"
  mkdir -p reswrap-smoke
  cp "$SRC/apps/calculator/bigcalc.gif" reswrap-smoke/
  exe=$(find_exe reswrap) || true
  if [ -n "$exe" ] &&
     ( cd reswrap-smoke && wine "$exe" -o icon.cpp bigcalc.gif ) >/dev/null 2>&1 &&
     grep -q 'const unsigned char bigcalc\[\]=' reswrap-smoke/icon.cpp; then
    echo "reswrap generated the bigcalc array"
  else
    echo "warning: reswrap did not generate the expected array" >&2
    fi

  echo
  echo "==> calculator under Wine for ${run_seconds} seconds"
  exe=$(find_exe calculator) || true
  if [ -n "$exe" ]; then
    rc=0
    timeout "$run_seconds" wine "$exe" >"$BUILD/calculator.wine.log" 2>&1 || rc=$?
    tail -n 5 "$BUILD/calculator.wine.log"
    if [ "$rc" -eq 124 ]; then
      echo "calculator stayed in its event loop (expected)"
    else
      echo "warning: calculator exited with status $rc" >&2
      fi
  else
    echo "note: no calculator binary was built"
    fi
  fi

if [ -n "$install_prefix" ]; then
  echo
  echo "==> install to $install_prefix"
  if [ "$use_cmake" = yes ]; then
    run cmake --install "$BUILD" --prefix "$cmake_install_prefix"
  else
    run make install
    fi
  fi

echo
echo "done. Build log: $LOG"
if [ "$trace" = yes ]; then
  echo
  echo "trace build: FXTRACE is compiled in; run with -tracelevel N or FOX_TRACE_LEVEL=N"
  echo "and redirect stderr, e.g. wine $BUILD/bin/dpi.exe -tracelevel 1023 2> trace.log"
  fi
