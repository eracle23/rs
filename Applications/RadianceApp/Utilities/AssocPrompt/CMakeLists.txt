cmake_minimum_required(VERSION 3.16)

project(AssocPrompt LANGUAGES CXX)

if(NOT WIN32)
  return()
endif()

add_executable(AssocPrompt WIN32 AssocPrompt.cpp)
target_compile_features(AssocPrompt PRIVATE cxx_std_17)
target_link_libraries(AssocPrompt PRIVATE shell32 ole32 advapi32 shlwapi)

# Place binaries under the top-level build bin directory for easy reference
set_target_properties(AssocPrompt PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
)

# Install into the application bin alongside the main launcher
if(DEFINED Slicer_INSTALL_BIN_DIR)
  install(TARGETS AssocPrompt RUNTIME DESTINATION ${Slicer_INSTALL_BIN_DIR} COMPONENT Runtime)
else()
  install(TARGETS AssocPrompt RUNTIME DESTINATION bin)
endif()
