foreach(prog hello button dpi popup click menus tooltip clipboard dnd icon bmpicon textspin combo colordlg fontdlg idle hello2 layout ratio header splitter foursplit gaugetest)
  add_executable(${prog} ${prog}.cpp)
  target_link_libraries(${prog} PRIVATE FOX::FOX m)
endforeach()

# Console tests: no display required
set(FOX_CONSOLE_TESTS codecs desktop dictest expression format glyphs half match math memmap minheritance parallel process region rex scan thread timefmt unicode units variant winmap xml)
foreach(prog ${FOX_CONSOLE_TESTS})
  add_executable(${prog} ${prog}.cpp)
  target_link_libraries(${prog} PRIVATE FOX::FOX m)
endforeach()

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
set(FOX_CTEST_PLAIN codecs desktop glyphs match minheritance region scan unicode winmap)
foreach(prog ${FOX_CTEST_SELFTEST})
  add_test(NAME ${prog} COMMAND ${prog} --no-dialogs --test WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()
foreach(prog ${FOX_CTEST_PLAIN})
  add_test(NAME ${prog} COMMAND ${prog} --no-dialogs WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()
add_test(NAME raster COMMAND raster --no-dialogs --raster-only WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set_tests_properties(${FOX_CTEST_SELFTEST} ${FOX_CTEST_PLAIN} raster PROPERTIES TIMEOUT 120)
