add_executable(foxterm
  Profiles.cpp
  TerminalTabBar.cpp
  TerminalWindow.cpp
  main.cpp)
target_link_libraries(foxterm PRIVATE FOX::FOX m)
