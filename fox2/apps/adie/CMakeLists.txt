add_executable(adie
  icons.cpp
  help.cpp
  Adie.cpp
  Commands.cpp
  FindInFiles.cpp
  HelpWindow.cpp
  main.cpp
  Modeline.cpp
  Preferences.cpp
  ReplaceInFiles.cpp
  ShellCommand.cpp
  ShellDialog.cpp
  Syntax.cpp
  SyntaxParser.cpp
  TextWindow.cpp)
target_link_libraries(adie PRIVATE FOX::FOX m)
