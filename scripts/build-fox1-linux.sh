#!/usr/bin/env bash
#
# Build the unmodified FOX 1.7.95 tree natively on Linux with the X11 backend.
#
# This is the baseline build of docs/general-plan.md: the original source tree
# stays untouched (`fox-1.7.95/` is read only) and the build is out-of-tree in
# tmp/build-fox1.  The shipped configure runs as it is, with the options a
# plain ./configure would use (OpenGL and CUPS on), so the result is the
# reference the fox2 work compares against.
#
#   ./scripts/build-fox1-linux.sh
#
# Options:
#   --opengl    enable OpenGL (the default; needs libgl-dev)
#   --no-opengl disable OpenGL
#   --no-cups   compile without CUPS support
#   --static    build a static library instead of the shared one
#   --debug     compile with -g -DDEBUG
#   --release   compile with -O2 -DNDEBUG
#   --jobs=N    parallel jobs (default: all cores)
#   --clean     delete the build directory first
#   --fast      skip configure when its inputs did not change
#   --run       run hello and button for 3 s after building
#   --cmake     use CMake + Ninja instead of Autotools
#               (build dir: tmp/build-fox1-cmake)
#   --install[=DIR] install after building (default prefix: /usr/local)
#   -h|--help   show this help
#
# Environment:
#   FOX_BUILD_DIR  build directory (default: tmp/build-fox1); use it under WSL
#                  to build on the Linux file system, e.g. ~/build/fox1
#
set -euo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
SRC="$ROOT/fox-1.7.95"
BUILD="${FOX_BUILD_DIR:-$ROOT/tmp/build-fox1}"
LOG="$BUILD/build.log"

jobs=$(nproc 2>/dev/null || echo 4)
with_opengl=yes
with_cups=yes
shared=yes
build_type=
clean=no
fast=no
run_test=no
use_cmake=no
install_prefix=

usage(){
  sed -n '2,32p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
  }

while [ $# -gt 0 ]; do
  case "$1" in
    --opengl)    with_opengl=yes ;;
    --no-opengl) with_opengl=no ;;
    --no-cups)   with_cups=no ;;
    --static)    shared=no ;;
    --debug)     build_type=debug ;;
    --release)   build_type=release ;;
    --clean)     clean=yes ;;
    --fast)      fast=yes ;;
    --run)       run_test=yes ;;
    --cmake)     use_cmake=yes ;;
    --install)
      if [ $# -ge 2 ] && [ "${2#-}" = "$2" ]; then
        install_prefix=$2; shift
      else
        install_prefix=/usr/local
        fi ;;
    --install=*) install_prefix=${1#*=}; [ -n "$install_prefix" ] || install_prefix=/usr/local ;;
    --jobs)      shift; jobs=$1 ;;
    --jobs=*)    jobs=${1#*=} ;;
    -h|--help)   usage; exit 0 ;;
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
  echo "error: this is the Linux build; in MSYS2 use ./scripts/build-fox1-mingw.sh" >&2
  exit 1
  fi

command -v g++ >/dev/null 2>&1 || {
  echo "error: g++ not found. Install the C++ compiler (gcc)." >&2
  exit 1
  }
command -v make >/dev/null 2>&1 || {
  echo "error: make not found. Install make." >&2
  exit 1
  }
[ -f "$SRC/configure" ] || {
  echo "error: $SRC/configure not found." >&2
  exit 1
  }
pkg-config --exists x11 2>/dev/null || {
  echo "error: X11 development files not found (pkg-config x11)." >&2
  echo "       install libx11-dev (Debian/Ubuntu) or libx11 (Arch)." >&2
  exit 1
  }

if [ "$run_test" = yes ] && [ -z "${DISPLAY-}" ]; then
  echo "error: --run needs an X display (DISPLAY is not set)." >&2
  exit 1
  fi

if [ "$use_cmake" = yes ]; then
  BUILD="${FOX_BUILD_DIR:-$ROOT/tmp/build-fox1-cmake}"
  LOG="$BUILD/build.log"
  command -v cmake >/dev/null 2>&1 || {
    echo "error: cmake not found. Install cmake and ninja." >&2
    exit 1
    }
  command -v ninja >/dev/null 2>&1 || {
    echo "error: ninja not found. Install cmake and ninja." >&2
    exit 1
    }

  if [ "$clean" = yes ]; then
    echo "==> removing $BUILD"
    rm -rf -- "$BUILD"
    fi
  mkdir -p -- "$BUILD"
  : >"$LOG"

  run(){
    echo "*** $*" >>"$LOG"
    "$@" 2>&1 | tee -a "$LOG"
    }

  # CMake needs no generated files, so the original tree is used directly.
  # The options are always passed so that they are reset when none is given.
  #
  # The codec tests call libtiff, libjpeg etc. directly.  CMake links the codec
  # libraries PRIVATE to the shared FOX library, so the test executables do not
  # inherit them and would fail with "DSO missing from command line".  Add the
  # same libraries to the executable link for the all-tests build.
  codec_libs=
  for pkg in libtiff-4 libjpeg libjxl zlib bzip2 liblzma; do
    if pkg-config --exists "$pkg"; then
      codec_libs+="$(pkg-config --libs "$pkg") "
      fi
    done
  if [ "$with_cups" = yes ] && pkg-config --exists cups; then
    codec_libs+="$(pkg-config --libs cups) "
    fi
  copts=(-G Ninja -DFOX_BUILD_ALL_TESTS=ON -DCMAKE_EXE_LINKER_FLAGS="$codec_libs")
  if [ "$with_opengl" = yes ]; then
    copts+=(-DFOX_WITH_OPENGL=ON)
  else
    copts+=(-DFOX_WITH_OPENGL=OFF)
    fi
  if [ "$with_cups" = yes ]; then
    copts+=(-DFOX_WITH_CUPS=ON)
  else
    copts+=(-DFOX_WITH_CUPS=OFF)
    fi
  if [ "$shared" = yes ]; then
    copts+=(-DFOX_BUILD_SHARED=ON)
  else
    copts+=(-DFOX_BUILD_SHARED=OFF)
    fi
  case "$build_type" in
    debug)   copts+=(-DCMAKE_BUILD_TYPE=Debug) ;;
    release) copts+=(-DCMAKE_BUILD_TYPE=Release) ;;
    *)       copts+=(-DCMAKE_BUILD_TYPE=) ;;
  esac

  # Ninja reruns CMake by itself when a CMakeLists.txt or Makefile.am changes
  if [ "$fast" = yes ] && [ -f "$BUILD/build.ninja" ] &&
     [ "$(cat "$BUILD/.cmake-options" 2>/dev/null)" = "${copts[*]}" ]; then
    echo "==> cmake configure skipped (options unchanged)"
  else
    echo "==> cmake configure: ${copts[*]}"
    run cmake -S "$SRC" -B "$BUILD" "${copts[@]}"
    printf '%s\n' "${copts[*]}" >"$BUILD/.cmake-options"
    fi

  echo "==> build reswrap, library and all test programs"
  run cmake --build "$BUILD" -j "$jobs"

  echo
  echo "==> artifacts"
  ls -l "$BUILD"/bin/* "$BUILD"/lib/*FOX* 2>/dev/null || true

  if [ "$run_test" = yes ]; then
    for prog in hello button; do
      echo
      echo "==> running $prog for 3 seconds"
      rc=0
      timeout 3 "$BUILD/bin/$prog" || rc=$?
      if [ "$rc" -eq 124 ]; then
        echo "$prog stayed in its event loop (expected)"
      else
        echo "warning: $prog exited with status $rc" >&2
        fi
      done
    fi

  if [ -n "$install_prefix" ]; then
    echo
    echo "==> install to $install_prefix"
    run cmake --install "$BUILD" --prefix "$cmake_install_prefix"
    fi

  echo
  echo "done. Build log: $LOG"
  exit 0
  fi

if [ "$clean" = yes ]; then
  echo "==> removing $BUILD"
  rm -rf -- "$BUILD"
  fi
mkdir -p -- "$BUILD"

opts=()
if [ "$with_opengl" = yes ]; then
  opts+=(--with-opengl=yes)
else
  opts+=(--with-opengl=no)
  fi
if [ "$with_cups" = yes ]; then
  opts+=(--enable-cups)
else
  opts+=(--disable-cups)
  fi
if [ -n "$install_prefix" ]; then
  opts+=(--prefix="$install_prefix")
  fi
if [ "$shared" = yes ]; then
  opts+=(--enable-shared --disable-static)
else
  opts+=(--disable-shared --enable-static)
  fi
case "$build_type" in
  debug)   opts+=(--enable-debug) ;;
  release) opts+=(--enable-release) ;;
esac

: >"$LOG"

run(){
  echo "*** $*" >>"$LOG"
  "$@" 2>&1 | tee -a "$LOG"
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

cd -- "$BUILD"

if [ "$fast" = yes ] && ! needs_configure; then
  echo "==> configure skipped (build directory is up to date)"
else
  echo "==> configure: $SRC/configure ${opts[*]}"
  run "$SRC/configure" "${opts[@]}"
  printf '%s\n' "${opts[*]}" >"$BUILD/.configure-options"

  # Compiler and linker flags end up inside the object files, and make does
  # not rebuild them when the flags change; start over in that case.
  echo "==> cleaning objects (configure was rerun)"
  run make -C utils clean
  run make -C lib clean
  run make -C tests clean
  fi

echo "==> build reswrap (host utility, generates icons)"
run make -j"$jobs" -C utils

echo "==> build library"
run make -j"$jobs" -C lib

echo "==> build all test programs"
run make -j"$jobs" -C tests

echo
echo "==> artifacts"
ls -l lib/.libs/*FOX* 2>/dev/null || ls -l lib/*FOX* 2>/dev/null || true
# Count the test programs rather than listing the whole directory
echo "test programs built: $(find tests -maxdepth 1 -type f -perm -u+x | wc -l)"

if [ "$run_test" = yes ]; then
  for prog in hello button; do
    echo
    echo "==> running $prog for 3 seconds"
    rc=0
    ( cd tests && PATH="$BUILD/lib/.libs:$PATH" timeout 3 "./$prog" ) || rc=$?
    if [ "$rc" -eq 124 ]; then
      echo "$prog stayed in its event loop (expected)"
    else
      echo "warning: $prog exited with status $rc" >&2
      fi
    done
  fi

if [ -n "$install_prefix" ]; then
  echo
  echo "==> install to $install_prefix"
  run make install
  fi

echo
echo "done. Build log: $LOG"
