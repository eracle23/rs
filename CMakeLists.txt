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

# Windows: 配置 NSIS/CPack 行为（不再修改上游 Slicer 源码）
if(WIN32)
  # 通过 CPack 工程配置文件在打包时注入/覆盖 Finish 页宏，避免与上游产生顺序冲突
  set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_SOURCE_DIR}/CMake/CPackProjectConfig.cmake")
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
set(Slicer_DEFAULT_HOME_MODULE "DICOM" CACHE STRING "Default home module" FORCE)
# 常用模块列表（移除 Data, CompareVolumes, SampleData）
set(Slicer_DEFAULT_FAVORITE_MODULES "Volumes, Models, SegmentEditor, Markups, Transforms")

#
# Ensure Slicer version components are available early (before SuperBuild/cache setup)
#
macro(_rs_extract_slicer_version_component component)
  if(NOT DEFINED Slicer_VERSION_${component})
    if(NOT EXISTS "${slicersources_SOURCE_DIR}/CMakeLists.txt")
      message(FATAL_ERROR "Cannot determine Slicer version because ${slicersources_SOURCE_DIR}/CMakeLists.txt is missing.")
    endif()
    file(STRINGS "${slicersources_SOURCE_DIR}/CMakeLists.txt" _rs_ver_line
      REGEX "set\\(Slicer_VERSION_${component} \"[0-9]+\"\\)")
    if(_rs_ver_line STREQUAL "")
      message(FATAL_ERROR "Failed to parse Slicer_VERSION_${component} from ${slicersources_SOURCE_DIR}/CMakeLists.txt")
    endif()
    string(REGEX REPLACE ".*\"([0-9]+)\".*" "\\1" _rs_ver_value "${_rs_ver_line}")
    set(Slicer_VERSION_${component} "${_rs_ver_value}" CACHE INTERNAL "Slicer version ${component}" FORCE)
  endif()
endmacro()
_rs_extract_slicer_version_component(MAJOR)
_rs_extract_slicer_version_component(MINOR)
_rs_extract_slicer_version_component(PATCH)

#
# Scripted modules: install path and default search paths
# ------------------------------------------------------
# 目标：将所有 Python/Scripted 模块（包含内置与打包扩展）统一安装到
#       lib/<App>-<ver>/qt-scripted-modules，并保留 Slicer-<ver> 兼容路径。
#
set(Slicer_INSTALL_QTSCRIPTEDMODULES_LIB_DIR
  "lib/${SlicerApp_APPLICATION_NAME}-${Slicer_VERSION_MAJOR}.${Slicer_VERSION_MINOR}/qt-scripted-modules"
  CACHE PATH "Install dir for Qt scripted modules" FORCE)

set(Slicer_ADDITIONAL_MODULE_PATHS
  "${CMAKE_INSTALL_PREFIX}/lib/${SlicerApp_APPLICATION_NAME}-${Slicer_VERSION_MAJOR}.${Slicer_VERSION_MINOR}/qt-scripted-modules"
  "${CMAKE_INSTALL_PREFIX}/lib/Slicer-${Slicer_VERSION_MAJOR}.${Slicer_VERSION_MINOR}/qt-scripted-modules"
  CACHE STRING "Additional module search paths" FORCE)




# Configure SuperBuild
set(SUPERBUILD_TOPLEVEL_PROJECT Slicer)

# Ensure inner Slicer build exports its UseSlicer.cmake path to bundled extensions.
set(Slicer_BINARY_DIR "${CMAKE_BINARY_DIR}/Slicer-build"
    CACHE PATH "Inner Slicer build tree" FORCE)
set(Slicer_USE_FILE "${Slicer_BINARY_DIR}/UseSlicer.cmake"
    CACHE FILEPATH "Path to UseSlicer.cmake" FORCE)
set(Slicer_USE_FILE_CONFIG "${Slicer_BINARY_DIR}/UseSlicer.cmake"
    CACHE FILEPATH "UseSlicer path embedded in extension-friendly config" FORCE)
set(CTK_DIR "${CMAKE_BINARY_DIR}/CTK-build"
    CACHE PATH "CTK build tree for CONFIG-mode consumers" FORCE)
set(SlicerExecutionModel_DIR "${CMAKE_BINARY_DIR}/SlicerExecutionModel-build"
    CACHE PATH "SlicerExecutionModel build tree for CONFIG-mode consumers" FORCE)

# Ensure Slicer SuperBuild also searches our local SuperBuild directory
# for any future custom External_*.cmake files.
set(EXTERNAL_PROJECT_ADDITIONAL_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/SuperBuild")
include(ExternalProjectDependency)
if(NOT DEFINED RS_SKIP_FETCH_EXT)
  set(RS_SKIP_FETCH_EXT OFF CACHE BOOL "Skip fetching bundled extensions (set ON to reuse cached dirs)" FORCE)
endif()
mark_as_superbuild(Slicer_INSTALL_QTSCRIPTEDMODULES_LIB_DIR:PATH)
mark_as_superbuild(Slicer_ADDITIONAL_MODULE_PATHS:STRING)
mark_as_superbuild(Slicer_EXTENSIONS_DIRBASENAME:STRING)
mark_as_superbuild(Slicer_BINARY_DIR:PATH)
mark_as_superbuild(Slicer_USE_FILE:FILEPATH)
mark_as_superbuild(Slicer_USE_FILE_CONFIG:FILEPATH)
mark_as_superbuild(CTK_DIR:PATH)
mark_as_superbuild(SlicerExecutionModel_DIR:PATH)
mark_as_superbuild(EXTERNAL_PROJECT_ADDITIONAL_DIRS:STRING)
mark_as_superbuild(CMAKE_INSTALL_PREFIX:PATH)
mark_as_superbuild(Slicer_USE_SimpleITK:BOOL)
mark_as_superbuild(Slicer_EXTENSION_INSTALL_DIRS:STRING)
mark_as_superbuild(CPACK_PROJECT_CONFIG_FILE:FILEPATH)

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
  set(Slicer_BUILD_WIN32_CONSOLE_LAUNCHER OFF CACHE BOOL "Hide console window for end-user launcher" FORCE)
endif()

option(Slicer_BUILD_DICOM_SUPPORT               "Build application with DICOM support"                ON)
set(Slicer_BUILD_DICOM_SUPPORT ON CACHE BOOL "Build application with DICOM support" FORCE)
option(Slicer_BUILD_DIFFUSION_SUPPORT           "Build application with Diffusion support"            OFF)
option(Slicer_BUILD_EXTENSIONMANAGER_SUPPORT    "Build application with ExtensionManager support"     OFF)
set(Slicer_BUILD_EXTENSIONMANAGER_SUPPORT ON CACHE BOOL "Enable extension manager support" FORCE)
option(Slicer_BUILD_MULTIVOLUME_SUPPORT         "Build application with MultiVolume support"          OFF)
set(Slicer_BUILD_MULTIVOLUME_SUPPORT ON CACHE BOOL "Build application with MultiVolume support" FORCE)
set(Slicer_BUILD_MultiVolumeExplorer ON CACHE BOOL "Build MultiVolumeExplorer module" FORCE)
set(Slicer_BUILD_MultiVolumeImporter ON CACHE BOOL "Build MultiVolumeImporter module" FORCE)
option(Slicer_BUILD_PARAMETERSERIALIZER_SUPPORT "Build application with parameter serializer support" OFF)
option(Slicer_USE_PYTHONQT                      "Build application with Python support"               ON)
set(Slicer_USE_PYTHONQT ON CACHE BOOL "Build application with Python support" FORCE)
set(Slicer_USE_PYTHONQT_WITH_OPENSSL ON CACHE BOOL "Bundle Python SSL support" FORCE)

# 国际化支持
option(Slicer_BUILD_I18N_SUPPORT                "Build application with internationalization support" ON)
set(Slicer_BUILD_I18N_SUPPORT ON CACHE BOOL "Build application with internationalization support" FORCE)
set(Slicer_LANGUAGES "zh_CN" CACHE STRING "Supported languages for internationalization" FORCE)
mark_as_superbuild(Slicer_BUILD_I18N_SUPPORT:BOOL)
mark_as_superbuild(Slicer_LANGUAGES:STRING)
option(Slicer_USE_QtTesting                     "Build application with QtTesting support"            OFF)
option(Slicer_USE_SimpleITK                     "Build application with SimpleITK support"            ON)
set(Slicer_USE_SimpleITK ON CACHE BOOL "Build application with SimpleITK support" FORCE)
set(Slicer_BUILD_SimpleFilters ON CACHE BOOL "Build SimpleFilters remote module" FORCE)
set(Slicer_USE_NUMPY ON CACHE BOOL "Bundle NumPy support" FORCE)
set(Slicer_USE_SCIPY ON CACHE BOOL "Bundle SciPy support" FORCE)
set(Slicer_USE_DCMTK_WITH_OPENSSL ON CACHE BOOL "Enable TLS support for DCMTK" FORCE)

option(Slicer_BUILD_BRAINSTOOLS                 "Build application with BRAINSTools module"           OFF)
set(Slicer_BUILD_BRAINSTOOLS ON CACHE BOOL "Build application with BRAINSTools module" FORCE)
option(Slicer_BUILD_DataStore                   "Build application with DataStore module"             OFF)
option(Slicer_BUILD_CompareVolumes              "Build application with ChangeTrackerPy module"       OFF)
set(Slicer_BUILD_CompareVolumes ON CACHE BOOL "Build application with ChangeTrackerPy module" FORCE)
option(Slicer_BUILD_LandmarkRegistration        "Build application with LandmarkRegistration module"  ON)
set(Slicer_BUILD_LandmarkRegistration ON CACHE BOOL "Build application with LandmarkRegistration module" FORCE)
option(Slicer_BUILD_SurfaceToolbox              "Build application with SurfaceToolbox module"        OFF)
set(Slicer_BUILD_CLI ON CACHE BOOL "Build core CLI modules" FORCE)

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
set(Slicer_QTSCRIPTEDMODULES_DISABLED "" CACHE STRING "Disable Slicer-branded scripted modules" FORCE)
list(REMOVE_ITEM Slicer_QTLOADABLEMODULES_DISABLED
  Data SubjectHierarchy SubjectHierarchyPlugins
  Volumes Models Transforms Markups VolumeRendering)
list(REMOVE_ITEM Slicer_QTSCRIPTEDMODULES_DISABLED
  LandmarkRegistration CompareVolumes SampleData)
list(REMOVE_ITEM Slicer_CLIMODULES_DISABLED
  "BRAINSFit")

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

set(_rs_ext_legacy_root "${CMAKE_BINARY_DIR}/_deps")
file(TO_CMAKE_PATH "${_rs_ext_legacy_root}" _rs_ext_legacy_root_norm)
set(_rs_pruned_extension_dirs)
foreach(_rs_ext_dir IN LISTS Slicer_EXTENSION_SOURCE_DIRS)
  file(TO_CMAKE_PATH "${_rs_ext_dir}" _rs_ext_dir_norm)
  if(_rs_ext_dir_norm MATCHES "^${_rs_ext_legacy_root_norm}(/|$)")
    message(STATUS "Skip legacy extension dir (using Ext/<name>): ${_rs_ext_dir}")
    continue()
  endif()
  list(APPEND _rs_pruned_extension_dirs "${_rs_ext_dir}")
endforeach()
set(Slicer_EXTENSION_SOURCE_DIRS "${_rs_pruned_extension_dirs}")
unset(_rs_pruned_extension_dirs)
unset(_rs_ext_legacy_root)
unset(_rs_ext_legacy_root_norm)

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

#------------------------------------------------------------------------------
# 中文翻译支持
# 将SlicerLanguageTranslations翻译文件编译并安装到应用中
#------------------------------------------------------------------------------
set(SLICER_TRANSLATIONS_DIR "${CMAKE_SOURCE_DIR}/SlicerLanguageTranslations-main/translations"
    CACHE PATH "SlicerLanguageTranslations翻译文件目录")
set(SLICER_TRANSLATIONS_LANGUAGES "zh-CN" CACHE STRING "要编译的翻译语言列表")
set(SLICER_TRANSLATIONS_COMPONENTS "Slicer;CTK" CACHE STRING "要编译的翻译组件列表")

mark_as_superbuild(SLICER_TRANSLATIONS_DIR:PATH)
mark_as_superbuild(SLICER_TRANSLATIONS_LANGUAGES:STRING)
mark_as_superbuild(SLICER_TRANSLATIONS_COMPONENTS:STRING)

if(EXISTS "${SLICER_TRANSLATIONS_DIR}" AND Slicer_BUILD_I18N_SUPPORT)
  message(STATUS "SlicerLanguageTranslations: ${SLICER_TRANSLATIONS_DIR}")
  message(STATUS "  Languages: ${SLICER_TRANSLATIONS_LANGUAGES}")
  message(STATUS "  Components: ${SLICER_TRANSLATIONS_COMPONENTS}")
endif()