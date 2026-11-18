add_executable(stopwatch
  ClockView.cpp
  History.cpp
  StopWatch.cpp
  main.cpp)
target_link_libraries(stopwatch PRIVATE FOX::FOX m)
