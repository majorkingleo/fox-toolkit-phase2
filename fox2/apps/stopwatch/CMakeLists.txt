add_executable(stopwatch
  ClockView.cpp
  History.cpp
  HistoryWindow.cpp
  StopWatch.cpp
  main.cpp)
target_link_libraries(stopwatch PRIVATE FOX::FOX m)

if(WIN32)
  target_link_libraries(stopwatch PRIVATE advapi32)
endif()
