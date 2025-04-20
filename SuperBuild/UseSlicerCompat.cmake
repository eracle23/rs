# Minimal compatibility layer for extensions expecting Slicer_USE_FILE and mark_as_superbuild
# - When building extensions against a Slicer custom app, Slicer_USE_FILE may be empty.
# - Some legacy extensions include(${Slicer_USE_FILE}) and call mark_as_superbuild(...).
# - This shim safely includes ExternalProjectDependency to provide mark_as_superbuild and friends.

if(DEFINED Slicer_CMAKE_DIR AND EXISTS "${Slicer_CMAKE_DIR}/ExternalProjectDependency.cmake")
  include("${Slicer_CMAKE_DIR}/ExternalProjectDependency.cmake")
endif()

