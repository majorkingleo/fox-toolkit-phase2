foreach(prog hello button dpi popup click menus tooltip clipboard dnd icon bmpicon textspin combo colordlg colordemo fontdlg rulerdemo gaugedemo idle hello2 layout ratio header splitter foursplit gaugetest switcher tabbook groupbox dialog wizard datatarget calendar dirlist iconlist table image imageviewer scribble dctest mditest shutter console glcanvas svg)
  add_executable(${prog} ${prog}.cpp)
  target_link_libraries(${prog} PRIVATE FOX::FOX m)
endforeach()

# bitmapviewer calls libtiff directly when it is available
add_executable(bitmapviewer bitmapviewer.cpp)
target_link_libraries(bitmapviewer PRIVATE FOX::FOX ${FOX_PRIVATE_LIBRARIES} m)

# gltest needs GLU for gluPerspective and gluLookAt
add_executable(gltest gltest.cpp)
target_link_libraries(gltest PRIVATE FOX::FOX ${FOX_GLU_LIBRARIES} m)

# Console tests: no display required
set(FOX_CONSOLE_TESTS codecs consolemodel desktop dictest expression format glyphs half match math memmap minheritance parallel process pty region rex scan terminalmodel thread timefmt unicode units variant winmap xml)
foreach(prog ${FOX_CONSOLE_TESTS})
  add_executable(${prog} ${prog}.cpp)
  target_link_libraries(${prog} PRIVATE FOX::FOX m)
endforeach()

# The terminal model test includes the private screen and parser headers
target_include_directories(terminalmodel PRIVATE ${PROJECT_SOURCE_DIR}/lib)
target_include_directories(pty PRIVATE ${PROJECT_SOURCE_DIR}/lib)

add_executable(chartmodel chartmodel.cpp)
target_link_libraries(chartmodel PRIVATE FOX::FOX m)

# Software rasterizer: --raster-only needs no display, otherwise it compares
# its buffer with the same scene drawn by FXDCWindow
add_executable(raster raster.cpp)
target_link_libraries(raster PRIVATE FOX::FOX m)
target_include_directories(raster PRIVATE ${PROJECT_SOURCE_DIR}/lib)

# CTest registration. Console tests need no display and no input. The ones
# with a --test mode run it; the others check themselves when started plainly.
# Working directory is the build directory, so temporary files stay out of the
# source tree.
set(FOX_CTEST_SELFTEST dictest expression format half math memmap parallel process rex thread timefmt units variant xml)
set(FOX_CTEST_PLAIN codecs consolemodel desktop glyphs match minheritance pty region scan terminalmodel unicode winmap)
foreach(prog ${FOX_CTEST_SELFTEST})
  add_test(NAME ${prog} COMMAND ${prog} --no-dialogs --test WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()
foreach(prog ${FOX_CTEST_PLAIN})
  add_test(NAME ${prog} COMMAND ${prog} --no-dialogs WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()
add_test(NAME raster COMMAND raster --no-dialogs --raster-only WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME chartmodel COMMAND chartmodel WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set_tests_properties(${FOX_CTEST_SELFTEST} ${FOX_CTEST_PLAIN} raster PROPERTIES TIMEOUT 120)
