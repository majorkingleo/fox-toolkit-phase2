# libFOX-1.7. The source list is read from Makefile.am, so the Autotools and
# the CMake build always compile the same files.

fox_read_makefile_sources("${CMAKE_CURRENT_SOURCE_DIR}/Makefile.am" FOX_LIB_SOURCES)

# Platform seam: exactly one implementation is compiled into the library
list(FILTER FOX_LIB_SOURCES EXCLUDE REGEX "_(win32|posix|unix|x11|wayland)\\.cpp$")
if(WIN32)
  list(APPEND FOX_LIB_SOURCES FXApp_win32.cpp FXWindow_win32.cpp FXFont_win32.cpp FXIODevice_win32.cpp FXMutex_win32.cpp FXAutoThreadStorageKey_win32.cpp FXPipe_win32.cpp FXDir_win32.cpp FXFile_win32.cpp FXProcess_win32.cpp FXStat_win32.cpp FXSystem_win32.cpp FXPath_win32.cpp FXThread_win32.cpp FXCursor_win32.cpp FXVisual_win32.cpp FXDCWindow_win32.cpp FXTopWindow_win32.cpp FXRegion_win32.cpp fxutils_win32.cpp fxkeyboard_win32.cpp FXStyle_win32.cpp FXImage_win32.cpp FXRootWindow_win32.cpp FXIcon_win32.cpp FXPopup_win32.cpp FXShell_win32.cpp FXToolTip_win32.cpp FXMainWindow_win32.cpp FXDragCorner_win32.cpp FXDesktopSettings_win32.cpp fxpriv_win32.cpp FXComposeContext_win32.cpp FXRegistry_win32.cpp FXCheckButton_win32.cpp FXSlider_win32.cpp FXSemaphore_win32.cpp FXMappedFile_win32.cpp FXSystemTime_win32.cpp fxosver_win32.cpp)
else()
  # POSIX and Unix modules are shared by the X11 and the Wayland platform
  list(APPEND FOX_LIB_SOURCES FXAutoThreadStorageKey_posix.cpp FXDir_posix.cpp FXFile_posix.cpp FXIODevice_posix.cpp FXMappedFile_posix.cpp FXMutex_posix.cpp FXPath_posix.cpp FXPipe_posix.cpp FXProcess_posix.cpp FXSemaphore_posix.cpp FXStat_posix.cpp FXSystemTime_posix.cpp FXSystem_posix.cpp FXThread_posix.cpp fxosver_posix.cpp fxutils_posix.cpp)
  list(APPEND FOX_LIB_SOURCES FXCheckButton_unix.cpp FXDesktopSettings_unix.cpp FXRegistry_unix.cpp FXShell_unix.cpp FXSlider_unix.cpp)
  if(FOX_PLATFORM STREQUAL "x11")
    list(APPEND FOX_LIB_SOURCES FXApp_x11.cpp FXComposeContext_x11.cpp FXCursor_x11.cpp FXDCWindow_x11.cpp FXDragCorner_x11.cpp FXFont_x11.cpp FXIcon_x11.cpp FXImage_x11.cpp FXMainWindow_x11.cpp FXPopup_x11.cpp FXRegion_x11.cpp FXRootWindow_x11.cpp FXStyle_x11.cpp FXToolTip_x11.cpp FXTopWindow_x11.cpp FXVisual_x11.cpp FXWindow_x11.cpp fxpriv_x11.cpp)
  elseif(FOX_PLATFORM STREQUAL "wayland")
    # Wayland modules; the set grows phase by phase, see docs/wayland-plan.md
    list(APPEND FOX_LIB_SOURCES FXRegion_wayland.cpp FXVisual_wayland.cpp FXImage_wayland.cpp FXIcon_wayland.cpp FXFont_wayland.cpp FXDCWindow_wayland.cpp)
  endif()
endif()

if(FOX_BUILD_SHARED)
  add_library(FOX SHARED ${FOX_LIB_SOURCES})
else()
  add_library(FOX STATIC ${FOX_LIB_SOURCES})
endif()
add_library(FOX::FOX ALIAS FOX)

set_target_properties(FOX PROPERTIES
  OUTPUT_NAME FOX-1.7
  VERSION ${PROJECT_VERSION}
  SOVERSION 0)

target_include_directories(FOX PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/fox-1.7>)
if(FOX_PUBLIC_INCLUDE_DIRS)
  target_include_directories(FOX SYSTEM PUBLIC ${FOX_PUBLIC_INCLUDE_DIRS})
endif()
if(FOX_PRIVATE_INCLUDE_DIRS)
  target_include_directories(FOX SYSTEM PRIVATE ${FOX_PRIVATE_INCLUDE_DIRS})
endif()

target_compile_definitions(FOX PUBLIC ${FOX_PUBLIC_DEFINITIONS})
target_link_libraries(FOX PUBLIC ${FOX_PUBLIC_LIBRARIES} PRIVATE ${FOX_PRIVATE_LIBRARIES})

if(WIN32)
  target_compile_definitions(FOX PRIVATE UNICODE _WINDOWS _USRDLL _CRT_SECURE_NO_WARNINGS)
  # Trace statements vanish with NDEBUG
  if(NOT FOX_FOXTRACE)
    target_compile_definitions(FOX PRIVATE NDEBUG)
  endif()
  # Users keep FXAPI empty, exactly like the Autotools build
  if(FOX_BUILD_SHARED)
    target_compile_definitions(FOX PRIVATE FOXDLL FOXDLL_EXPORTS)
    # Not every exported class carries FXAPI; libtool exports all symbols
    # of the DLL, so do the same with the GNU linker.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_link_options(FOX PRIVATE -Wl,--export-all-symbols)
    endif()
  endif()
endif()

install(TARGETS FOX
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
