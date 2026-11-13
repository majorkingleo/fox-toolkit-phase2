# libFOX-1.7. The source list is read from Makefile.am, so the Autotools and
# the CMake build always compile the same files.

fox_read_makefile_sources("${CMAKE_CURRENT_SOURCE_DIR}/Makefile.am" FOX_LIB_SOURCES)

# Platform seam: exactly one implementation is compiled into the library
list(FILTER FOX_LIB_SOURCES EXCLUDE REGEX "_(win32|x11)\\.cpp$")
if(WIN32)
  list(APPEND FOX_LIB_SOURCES FXApp_win32.cpp FXWindow_win32.cpp FXFont_win32.cpp FXIODevice_win32.cpp FXMutex_win32.cpp FXAutoThreadStorageKey_win32.cpp FXPipe_win32.cpp FXDir_win32.cpp FXFile_win32.cpp FXProcess_win32.cpp FXStat_win32.cpp FXSystem_win32.cpp FXPath_win32.cpp FXThread_win32.cpp FXCursor_win32.cpp FXVisual_win32.cpp FXDCWindow_win32.cpp FXTopWindow_win32.cpp FXRegion_win32.cpp fxutils_win32.cpp fxkeyboard_win32.cpp FXStyle_win32.cpp FXImage_win32.cpp FXRootWindow_win32.cpp FXIcon_win32.cpp FXPopup_win32.cpp FXShell_win32.cpp FXToolTip_win32.cpp FXMainWindow_win32.cpp FXDragCorner_win32.cpp FXDesktopSettings_win32.cpp fxpriv_win32.cpp FXComposeContext_win32.cpp FXRegistry_win32.cpp FXCheckButton_win32.cpp FXSlider_win32.cpp FXSemaphore_win32.cpp FXMappedFile_win32.cpp FXSystemTime_win32.cpp fxosver_win32.cpp)
else()
  list(APPEND FOX_LIB_SOURCES FXApp_x11.cpp FXWindow_x11.cpp FXFont_x11.cpp FXIODevice_x11.cpp FXMutex_x11.cpp FXAutoThreadStorageKey_x11.cpp FXPipe_x11.cpp FXDir_x11.cpp FXFile_x11.cpp FXProcess_x11.cpp FXStat_x11.cpp FXSystem_x11.cpp FXPath_x11.cpp FXThread_x11.cpp FXCursor_x11.cpp FXVisual_x11.cpp FXDCWindow_x11.cpp FXTopWindow_x11.cpp FXRegion_x11.cpp fxutils_x11.cpp FXStyle_x11.cpp FXImage_x11.cpp FXRootWindow_x11.cpp FXIcon_x11.cpp FXPopup_x11.cpp FXShell_x11.cpp FXToolTip_x11.cpp FXMainWindow_x11.cpp FXDragCorner_x11.cpp FXDesktopSettings_x11.cpp fxpriv_x11.cpp FXComposeContext_x11.cpp FXRegistry_x11.cpp FXCheckButton_x11.cpp FXSlider_x11.cpp FXSemaphore_x11.cpp FXMappedFile_x11.cpp FXSystemTime_x11.cpp fxosver_x11.cpp)
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
