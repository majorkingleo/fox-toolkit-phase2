add_executable(foxterm
  TerminalTabBar.cpp
  TerminalWindow.cpp
  main.cpp)
target_link_libraries(foxterm PRIVATE FOX::FOX m)
