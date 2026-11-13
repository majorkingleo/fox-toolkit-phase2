add_executable(reswrap reswrap.cpp)
if(WIN32)
  target_compile_definitions(reswrap PRIVATE WIN32)
endif()

install(TARGETS reswrap RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
