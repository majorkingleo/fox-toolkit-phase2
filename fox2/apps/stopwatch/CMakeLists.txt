add_executable(stopwatch
  ClockView.cpp
  History.cpp
  HistoryWindow.cpp
  StopWatch.cpp
  main.cpp)
target_link_libraries(stopwatch PRIVATE FOX::FOX m)

if(WIN32)
  set_target_properties(stopwatch PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
