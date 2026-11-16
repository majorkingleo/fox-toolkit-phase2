add_executable(ControlPanel
  icons.cpp
  ControlPanel.cpp)
target_link_libraries(ControlPanel PRIVATE FOX::FOX m)
