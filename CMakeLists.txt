cmake_minimum_required(VERSION 3.16.3)

# Enable C++14
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set (EP_GIT_PROTOCOL "https")

# Slicer sources
include(FetchContent)
if(NOT DEFINED slicersources_SOURCE_DIR)
  # Download Slicer sources and set variables slicersources_SOURCE_DIR and slicersources_BINARY_DIR
  FetchContent_Populate(slicersources
    GIT_REPOSITORY ${EP_GIT_PROTOCOL}://github.com/Slicer/Slicer
    GIT_TAG        v5.8.1
    GIT_PROGRESS   1
    )
endif()
# Respect externally provided binary dir; otherwise, default to subbuild
if(NOT DEFINED slicersources_BINARY_DIR)
  set(slicersources_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/slicersources-subbuild)
endif()

 # Note: Do not modify upstream Slicer sources; instead rely on cache
 # variables (e.g., `Slicer_EXTENSIONS_DIRBASENAME`) forwarded via
 # `mark_as_superbuild(...)` and normal add_subdirectory cache scoping.

# macOS initialization
set(CMAKE_MODULE_PATH ${slicersources_SOURCE_DIR}/CMake ${CMAKE_MODULE_PATH})
include(SlicerInitializeOSXVariables)

project(AliceSuite)

# Configure Application
set(Slicer_APPLICATIONS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Applications)
set(Slicer_MAIN_PROJECT "RadianceApp")

# Set organization
set(Slicer_ORGANIZATION_DOMAIN "alicelabs.com")
set(Slicer_ORGANIZATION_NAME   "Alice Labs")

# Default home and favorite modules
set(SlicerApp_APPLICATION_NAME "Alice")
set(SlicerApp_APPLICATION_DISPLAY_NAME "Alice Studio")
set(Slicer_DISCLAIMER_AT_STARTUP "Alice Studio is intended for research use only and is not cleared for diagnostic use.")
set(Slicer_EXTENSIONS_DIRBASENAME "AliceExtensions" CACHE STRING "Extensions directory name")
if(Slicer_EXTENSIONS_DIRBASENAME STREQUAL "")
  message(FATAL_ERROR "Slicer_EXTENSIONS_DIRBASENAME 为空。请在 SuperBuild 前传或用 -D 显式设置。")
endif()
set(Slicer_DEFAULT_HOME_MODULE "Data" CACHE STRING "Default home module" FORCE)
# 去掉自定义 Home 模块后，更新常用模块列表（不再包含 Home）
set(Slicer_DEFAULT_FAVORITE_MODULES "Data, SegmentEditor, VolumeRendering, Transforms")

# Configure SuperBuild
set(SUPERBUILD_TOPLEVEL_PROJECT Slicer)
# Ensure Slicer SuperBuild also searches our local SuperBuild directory
# for any future custom External_*.cmake files.
set(EXTERNAL_PROJECT_ADDITIONAL_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/SuperBuild")
include(ExternalProjectDependency)
mark_as_superbuild(Slicer_EXTENSIONS_DIRBASENAME:STRING)
mark_as_superbuild(EXTERNAL_PROJECT_ADDITIONAL_DIRS:STRING)
mark_as_superbuild(CMAKE_INSTALL_PREFIX:PATH)
mark_as_superbuild(Slicer_USE_SimpleITK:BOOL)
mark_as_superbuild(Slicer_EXTENSION_INSTALL_DIRS:STRING)

# Additional Slicer dependencies looked up in EXTERNAL_PROJECT_ADDITIONAL_DIR
set(Slicer_ADDITIONAL_DEPENDENCIES
  )

#  Enable listed remote modules from ITK
set(Slicer_ITK_ADDITIONAL_MODULES
  )

if(NOT CMAKE_CONFIGURATION_TYPES)
  set(Slicer_DEFAULT_BUILD_TYPE "Release")
endif()
include(SlicerInitializeBuildType)
include(SlicerInitializeReleaseType)

# Set application bundle identifier for macOS
if(APPLE)
  set(Slicer_MACOSX_BUNDLE_GUI_IDENTIFIER "com.radiancelabs.radiancesuite")
endif()

# Installation folder and admin account requirement for Windows
if(WIN32)
  # Note: To avoid escaping issue, make sure to use forward slash when setting
  #       "Slicer_CPACK_NSIS_INSTALL_ROOT". It is replaced by "\\\\" in SlicerCPack.
  set(Slicer_CPACK_NSIS_INSTALL_REQUIRES_ADMIN_ACCOUNT ON)
  if(Slicer_CPACK_NSIS_INSTALL_REQUIRES_ADMIN_ACCOUNT)
    # User has administrative privileges, therefore we can install to shared folder
    # "C:\Program Files" or "C:\Program Files (x86)".
    if(CMAKE_CL_64)
      set(Slicer_CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    else()
      set(Slicer_CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
    endif()
  else()
    # We do not require administrative privileges, therefore we install to user folder
    # "C:\Users\<username>\AppData\Local".
    set(Slicer_CPACK_NSIS_INSTALL_ROOT "$LOCALAPPDATA/${Slicer_ORGANIZATION_NAME}")
  endif()
endif()

# Slicer options
option(BUILD_TESTING                            "Build application test suite"                        ON)
option(Slicer_BUILD_APPLICATIONUPDATE_SUPPORT   "Build application update support"                    OFF)
option(Slicer_BUILD_DOCUMENTATION               "Build documentation (Doxygen, sphinx, ...)"          OFF)
if(WIN32)
  option(Slicer_BUILD_WIN32_CONSOLE_LAUNCHER    "Build ${PROJECT_NAME} launcher executable as a console app on windows (displays console at application start)" OFF)
  option(Slicer_BUILD_WIN32_CONSOLE             "Build application executable as a console app (allows capturing and piping console output)" ON)
endif()

option(Slicer_BUILD_DICOM_SUPPORT               "Build application with DICOM support"                ON)
option(Slicer_BUILD_DIFFUSION_SUPPORT           "Build application with Diffusion support"            OFF)
option(Slicer_BUILD_EXTENSIONMANAGER_SUPPORT    "Build application with ExtensionManager support"     OFF)
option(Slicer_BUILD_MULTIVOLUME_SUPPORT         "Build application with MultiVolume support"          OFF)
option(Slicer_BUILD_PARAMETERSERIALIZER_SUPPORT "Build application with parameter serializer support" OFF)
option(Slicer_USE_PYTHONQT                      "Build application with Python support"               ON)
option(Slicer_USE_QtTesting                     "Build application with QtTesting support"            OFF)
option(Slicer_USE_SimpleITK                     "Build application with SimpleITK support"            ON)
set(Slicer_USE_SimpleITK ON CACHE BOOL "Build application with SimpleITK support" FORCE)

option(Slicer_BUILD_BRAINSTOOLS                 "Build application with BRAINSTools module"           OFF)
option(Slicer_BUILD_DataStore                   "Build application with DataStore module"             OFF)
option(Slicer_BUILD_CompareVolumes              "Build application with ChangeTrackerPy module"       OFF)
option(Slicer_BUILD_LandmarkRegistration        "Build application with LandmarkRegistration module"  OFF)
option(Slicer_BUILD_SurfaceToolbox              "Build application with SurfaceToolbox module"        OFF)

# Enable Slicer built-in modules
set(Slicer_CLIMODULES_ENABLED
  ResampleDTIVolume             # Needed by ResampleScalarVectorDWIVolume
  ResampleScalarVectorDWIVolume # Depends on DiffusionApplications, needed by CropVolume
  )
set(Slicer_QTLOADABLEMODULES_ENABLED
  )
set(Slicer_QTSCRIPTEDMODULES_ENABLED
  )

# Disable Slicer built-in modules
set(Slicer_CLIMODULES_DISABLED
  )
set(Slicer_QTLOADABLEMODULES_DISABLED "SlicerWelcome" CACHE STRING "Disable Slicer-branded loadable modules" FORCE)
set(Slicer_QTSCRIPTEDMODULES_DISABLED "SampleData" CACHE STRING "Disable Slicer-branded scripted modules" FORCE)

# Enable/Disable Slicer custom modules: To create a new module, use the SlicerExtensionWizard.
set(Slicer_EXTENSION_SOURCE_DIRS
  #${CMAKE_CURRENT_SOURCE_DIR}/Modules/CLI/MyCLIModule
  #${CMAKE_CURRENT_SOURCE_DIR}/Modules/Loadable/MyLoadableModule
  # Removed Home scripted module to rely on Welcome + custom layout only
  )

# Add remote extension source directories

# SlicerCustomAppUtilities
set(extension_name "SlicerCustomAppUtilities")
set(${extension_name}_SOURCE_DIR "${CMAKE_BINARY_DIR}/${extension_name}")
FetchContent_Populate(${extension_name}
  SOURCE_DIR     ${${extension_name}_SOURCE_DIR}
  GIT_REPOSITORY ${EP_GIT_PROTOCOL}://github.com/KitwareMedical/SlicerCustomAppUtilities.git
  GIT_TAG        1d984a2c9143e2617ff1ffa9d86c51e07dc6321e
  GIT_PROGRESS   1
  QUIET
  )
message(STATUS "Remote - ${extension_name} [OK]")
list(APPEND Slicer_EXTENSION_SOURCE_DIRS ${${extension_name}_SOURCE_DIR}/Modules/Scripted/SlicerCustomAppUtilities)

# 打包第三方扩展（根据 Docs/plugin.md）
include(${CMAKE_SOURCE_DIR}/SuperBuild/Extensions.cmake)
list(REMOVE_DUPLICATES Slicer_EXTENSION_SOURCE_DIRS)

# 将扩展源码路径显式传递给 Slicer 超级构建（需在最终值确定后调用）
mark_as_superbuild(Slicer_EXTENSION_SOURCE_DIRS:STRING)

# 确保子目录（Slicer）能读取到最终值：写入缓存
set(Slicer_EXTENSION_SOURCE_DIRS "${Slicer_EXTENSION_SOURCE_DIRS}" CACHE STRING "Bundled extension source dirs" FORCE)

# 追加每个扩展的 SuperBuild/Superbuild 目录到 EXTERNAL_PROJECT_ADDITIONAL_DIRS，
# 确保像 External_dcm2niix.cmake 这类外部依赖脚本可被内层配置找到。
foreach(_ext_dir IN LISTS Slicer_EXTENSION_SOURCE_DIRS)
  if(EXISTS "${_ext_dir}/SuperBuild")
    list(APPEND EXTERNAL_PROJECT_ADDITIONAL_DIRS "${_ext_dir}/SuperBuild")
  elseif(EXISTS "${_ext_dir}/Superbuild")
    list(APPEND EXTERNAL_PROJECT_ADDITIONAL_DIRS "${_ext_dir}/Superbuild")
  endif()
endforeach()
list(REMOVE_DUPLICATES EXTERNAL_PROJECT_ADDITIONAL_DIRS)
set(EXTERNAL_PROJECT_ADDITIONAL_DIRS "${EXTERNAL_PROJECT_ADDITIONAL_DIRS}" CACHE STRING "Additional ExternalProject search dirs" FORCE)

# SlicerOpenIGTLink
#set(extension_name "SlicerOpenIGTLink")
#set(${extension_name}_SOURCE_DIR "${CMAKE_BINARY_DIR}/${extension_name}")
#FetchContent_Populate(${extension_name}
#  SOURCE_DIR     ${${extension_name}_SOURCE_DIR}
#  GIT_REPOSITORY ${EP_GIT_PROTOCOL}://github.com/openigtlink/SlicerOpenIGTLink.git
#  GIT_TAG        2b92f7b1ffe02403109b671f28424e8770e902a0
#  GIT_PROGRESS   1
#  QUIET
#  )
#message(STATUS "Remote - ${extension_name} [OK]")
#list(APPEND Slicer_EXTENSION_SOURCE_DIRS ${${extension_name}_SOURCE_DIR})


# Pre-installed extension directories to bundle (Plan B)
# Note: Provide install-tree roots; elements separated by ';'
set(Slicer_EXTENSION_INSTALL_DIRS
  "C:/S/ext-dcm2nii-install"
  CACHE STRING "Pre-installed extensions to bundle" FORCE)
message(STATUS "Bundled pre-installed extensions: ${Slicer_EXTENSION_INSTALL_DIRS}")

# Add Slicer sources
add_subdirectory(${slicersources_SOURCE_DIR} ${slicersources_BINARY_DIR})

# Convenience target to avoid triggering the mother 'Slicer' target by mistake
add_custom_target(Slicer-fast
  DEPENDS Slicer-configure Slicer-build Slicer-install)
