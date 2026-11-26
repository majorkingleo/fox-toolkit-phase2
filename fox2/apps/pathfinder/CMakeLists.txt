set(ICON_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${ICON_GENERATED_DIR}")
file(GLOB ICON_FILES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.gif"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.bmp")
set(ICONS_H "${ICON_GENERATED_DIR}/icons.h")
set(ICONS_CPP "${ICON_GENERATED_DIR}/icons.cpp")

add_custom_command(
  OUTPUT "${ICONS_H}" "${ICONS_CPP}"
  COMMAND $<TARGET_FILE:reswrap> -h -o icons.h ${ICON_FILES}
  COMMAND $<TARGET_FILE:reswrap> -i icons.h -o icons.cpp ${ICON_FILES}
  DEPENDS reswrap ${ICON_FILES}
  WORKING_DIRECTORY "${ICON_GENERATED_DIR}"
  VERBATIM)

add_executable(PathFinder
  "${ICONS_CPP}"
  "${ICONS_H}"
  CopyDialog.cpp
  PathFinder.cpp
  Preferences.cpp
  PropertyDialog.cpp)
target_include_directories(PathFinder PRIVATE "${ICON_GENERATED_DIR}")
target_link_libraries(PathFinder PRIVATE FOX::FOX m)
