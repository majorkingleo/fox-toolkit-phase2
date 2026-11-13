# Test programs. hello and button are the baseline; the others are built with
# -DFOX_BUILD_ALL_TESTS=ON or by naming them, e.g. "ninja image".

set(FOX_BASELINE_TESTS hello button)

set(FOX_ALL_TESTS
  bitmapviewer button calendar codecs console datatarget dctest dialog dirlist
  dictest expression format foursplit gaugetest groupbox half header hello hello2
  iconlist image imageviewer layout match math mditest memmap minheritance
  parallel process ratio rex scan scribble shutter splitter switcher tabbook
  table thread timefmt unicode units variant wizard xml)

# gltest needs OpenGL and GLU
if("HAVE_GL_H=1" IN_LIST FOX_PUBLIC_DEFINITIONS AND FOX_GLU_LIBRARIES)
  list(APPEND FOX_ALL_TESTS gltest)
endif()

foreach(prog IN LISTS FOX_ALL_TESTS)
  add_executable(${prog} ${prog}.cpp)
  target_link_libraries(${prog} PRIVATE FOX::FOX m)
  if(NOT FOX_BUILD_ALL_TESTS AND NOT prog IN_LIST FOX_BASELINE_TESTS)
    set_target_properties(${prog} PROPERTIES EXCLUDE_FROM_ALL TRUE)
  endif()
endforeach()

if(TARGET gltest)
  target_link_libraries(gltest PRIVATE ${FOX_GLU_LIBRARIES})
endif()
