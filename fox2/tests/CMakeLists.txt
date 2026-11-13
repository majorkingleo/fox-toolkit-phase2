foreach(prog hello button dpi popup click)
  add_executable(${prog} ${prog}.cpp)
  target_link_libraries(${prog} PRIVATE FOX::FOX m)
endforeach()

# Console tests: no display required
set(FOX_CONSOLE_TESTS codecs dictest expression format glyphs half match math memmap minheritance parallel process region rex scan thread timefmt unicode units variant winmap xml)
foreach(prog ${FOX_CONSOLE_TESTS})
  add_executable(${prog} ${prog}.cpp)
  target_link_libraries(${prog} PRIVATE FOX::FOX m)
endforeach()

# Software rasterizer: --raster-only needs no display, otherwise it compares
# its buffer with the same scene drawn by FXDCWindow
add_executable(raster raster.cpp)
target_link_libraries(raster PRIVATE FOX::FOX m)
target_include_directories(raster PRIVATE ${PROJECT_SOURCE_DIR}/lib)
