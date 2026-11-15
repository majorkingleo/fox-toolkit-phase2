add_executable(calculator
  icons.cpp
  Calculator.cpp
  HelpWindow.cpp
  Preferences.cpp
  help.cpp
  main.cpp)
target_link_libraries(calculator PRIVATE FOX::FOX m)
