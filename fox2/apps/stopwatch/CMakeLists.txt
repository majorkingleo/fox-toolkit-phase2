add_executable(stopwatch
  StopWatch.cpp
  main.cpp)
target_link_libraries(stopwatch PRIVATE FOX::FOX m)
