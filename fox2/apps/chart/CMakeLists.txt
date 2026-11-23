set(CHART_SOURCES
  icons.cpp
  chartutils.cpp
  FXAxis.cpp
  FXChart.cpp
  FXData.cpp
  FXCurve.cpp
  FX2DChart.cpp
  FX2DPlot.cpp)

add_executable(charttest
  ${CHART_SOURCES}
  live/LiveSource.cpp
  charttest.cpp)
target_link_libraries(charttest PRIVATE FOX::FOX m)

add_executable(chartdemo
  ${CHART_SOURCES}
  CSVReader.h
  CSVReader.cpp
  chartdemo.cpp)
target_link_libraries(chartdemo PRIVATE FOX::FOX m)

if(WIN32)
  set_target_properties(chartdemo PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
