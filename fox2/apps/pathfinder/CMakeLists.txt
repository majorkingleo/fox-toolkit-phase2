add_executable(PathFinder
  icons.cpp
  CopyDialog.cpp
  PathFinder.cpp
  Preferences.cpp
  PropertyDialog.cpp)
target_link_libraries(PathFinder PRIVATE FOX::FOX m)
