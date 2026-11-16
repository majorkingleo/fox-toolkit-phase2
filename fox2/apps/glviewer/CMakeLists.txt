add_executable(glviewer
  FXGLCone.cpp
  FXGLCube.cpp
  FXGLCylinder.cpp
  FXGLLine.cpp
  FXGLPoint.cpp
  FXGLShape.cpp
  FXGLSphere.cpp
  FXGLTriangleMesh.cpp
  glviewer.cpp)
target_link_libraries(glviewer PRIVATE FOX::FOX ${FOX_GLU_LIBRARIES} m)
