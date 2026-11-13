foreach(prog hello button dpi)
  add_executable(${prog} ${prog}.cpp)
  target_link_libraries(${prog} PRIVATE FOX::FOX m)
endforeach()

# Console tests: no display required
set(FOX_CONSOLE_TESTS codecs dictest expression format half match math memmap minheritance parallel process rex scan thread timefmt unicode units variant xml)
foreach(prog ${FOX_CONSOLE_TESTS})
  add_executable(${prog} ${prog}.cpp)
  target_link_libraries(${prog} PRIVATE FOX::FOX m)
endforeach()
