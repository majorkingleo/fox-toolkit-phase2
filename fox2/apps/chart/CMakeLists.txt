add_executable(charttest
  icons.cpp
  chartutils.cpp
  FXAxis.cpp
  FXChart.cpp
  FXCurve.cpp
  FX2DChart.cpp
  FX2DPlot.cpp
  charttest.cpp)
target_link_libraries(charttest PRIVATE FOX::FOX m)
