set(CHART_ICON_SVGS
  icons/x-axis-label.svg
  icons/y-axis-label.svg
  icons/reset-zoom.svg
  icons/zoom.svg
  icons/data-series.svg
  icons/crop.svg
  icons/drag.svg
  icons/detach.svg
  icons/dark/x-axis-label.svg
  icons/dark/y-axis-label.svg
  icons/dark/reset-zoom.svg
  icons/dark/zoom.svg
  icons/dark/data-series.svg
  icons/dark/crop.svg
  icons/dark/drag.svg
  icons/dark/detach.svg)

set(CHART_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${CHART_GENERATED_DIR}")
set(CHART_ICONS_H "${CHART_GENERATED_DIR}/icons.h")
set(CHART_ICONS_CPP "${CHART_GENERATED_DIR}/icons.cpp")

set(CHART_ICON_RESOURCES
  -r chart_x_axis_label icons/x-axis-label.svg
  -r chart_y_axis_label icons/y-axis-label.svg
  -r chart_reset_zoom icons/reset-zoom.svg
  -r chart_zoom icons/zoom.svg
  -r chart_data_series icons/data-series.svg
  -r chart_crop icons/crop.svg
  -r chart_drag icons/drag.svg
  -r chart_detach icons/detach.svg
  -r chart_dark_x_axis_label icons/dark/x-axis-label.svg
  -r chart_dark_y_axis_label icons/dark/y-axis-label.svg
  -r chart_dark_reset_zoom icons/dark/reset-zoom.svg
  -r chart_dark_zoom icons/dark/zoom.svg
  -r chart_dark_data_series icons/dark/data-series.svg
  -r chart_dark_crop icons/dark/crop.svg
  -r chart_dark_drag icons/dark/drag.svg
  -r chart_dark_detach icons/dark/detach.svg)

add_custom_command(
  OUTPUT "${CHART_ICONS_H}" "${CHART_ICONS_CPP}"
  COMMAND $<TARGET_FILE:reswrap> -h -o "${CHART_ICONS_H}" -r marble marble.bmp
  COMMAND $<TARGET_FILE:reswrap> -h -a -oa "${CHART_ICONS_H}" ${CHART_ICON_RESOURCES}
  COMMAND $<TARGET_FILE:reswrap> -s -i "${CHART_ICONS_H}" -o "${CHART_ICONS_CPP}" -r marble marble.bmp
  COMMAND $<TARGET_FILE:reswrap> -s -a -oa "${CHART_ICONS_CPP}" ${CHART_ICON_RESOURCES}
  DEPENDS reswrap marble.bmp ${CHART_ICON_SVGS}
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  VERBATIM)

set(CHART_SOURCES
  "${CHART_ICONS_CPP}"
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
target_include_directories(charttest PRIVATE "${CHART_GENERATED_DIR}")
target_link_libraries(charttest PRIVATE FOX::FOX m)

add_executable(chartdemo
  ${CHART_SOURCES}
  "${CHART_ICONS_H}"
  CSVReader.h
  CSVReader.cpp
  ChartDemoWindow.h
  ChartDetachedWindow.h
  ChartDetachedWindow.cpp
  ChartDemoUtils.h
  ChartDemoUtils.cpp
  ChartTabItem.h
  ChartTabItem.cpp
  chartdemo.cpp
  chartdemo_main.cpp)
target_include_directories(chartdemo PRIVATE "${CHART_GENERATED_DIR}")
target_link_libraries(chartdemo PRIVATE FOX::FOX m)
