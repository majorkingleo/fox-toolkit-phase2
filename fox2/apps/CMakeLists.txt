add_subdirectory(calculator)
add_subdirectory(controlpanel)
add_subdirectory(chart)
add_subdirectory(pathfinder)
add_subdirectory(adie)
add_subdirectory(glviewer)

install(TARGETS calculator ControlPanel charttest PathFinder adie glviewer
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES
  calculator/calculator.1
  controlpanel/ControlPanel.1
  pathfinder/PathFinder.1
  adie/adie.1
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(PROGRAMS adie/Adie.stx DESTINATION ${CMAKE_INSTALL_BINDIR})
