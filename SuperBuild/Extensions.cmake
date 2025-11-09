include(FetchContent)

# 兼容 CMake 4.1 的 FetchContent_Populate 弃用告警（CMP0169）
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

# 可选：离线/本地模式。如果用户已通过命令行传入本地扩展路径，
# 可设置 -DRS_SKIP_FETCH_EXT=ON 跳过远端抓取，直接使用 Slicer_EXTENSION_SOURCE_DIRS。
if(DEFINED RS_SKIP_FETCH_EXT AND RS_SKIP_FETCH_EXT)
  message(STATUS "RS_SKIP_FETCH_EXT=ON: Skip fetching bundled extensions.")
  message(STATUS "Using Slicer_EXTENSION_SOURCE_DIRS=${Slicer_EXTENSION_SOURCE_DIRS}")
  return()
endif()

# 将外部扩展仓库以源码形式打包进应用构建中。
# 生成的路径通过 Slicer_EXTENSION_SOURCE_DIRS 传递给 Slicer 超级构建。
# Ensure every extension configure sees CTK/SEM before project().
set(_rs_ext_init "${CMAKE_BINARY_DIR}/E/_rs_ext_init.cmake")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/E")
file(WRITE "${_rs_ext_init}" [=[
# Radiance/Slicer extension init guard (runs before project())
# Only run when Slicer_DIR points to the extension inner build (E directory).
if(DEFINED Slicer_DIR AND NOT "${Slicer_DIR}" STREQUAL "" AND EXISTS "${Slicer_DIR}/SlicerConfig.cmake")
  set(_rs_superbuild_root "")
  get_filename_component(_rs_superbuild_root "${Slicer_DIR}" DIRECTORY) # .../Slicer-build
  if(_rs_superbuild_root)
    get_filename_component(_rs_superbuild_root "${_rs_superbuild_root}" DIRECTORY) # superbuild root
  endif()

  # 1) Locate CTK_DIR and include UseCTK for macro availability.
  if(NOT DEFINED CTK_DIR)
    set(_rs_ctk_candidates
      "$ENV{CTK_DIR}"
      "${_rs_superbuild_root}/CTK-build"
      "${_rs_superbuild_root}/../CTK-build"
      "${CMAKE_BINARY_DIR}/../CTK-build"
      "${CMAKE_BINARY_DIR}/../../CTK-build"
    )
    foreach(_p IN LISTS _rs_ctk_candidates)
      if(_p AND EXISTS "${_p}/CTKConfig.cmake")
        set(CTK_DIR "${_p}" CACHE PATH "CTK build tree" FORCE)
        break()
      endif()
    endforeach()
  endif()
  if(NOT DEFINED CTK_SOURCE_DIR AND _rs_superbuild_root)
    set(_rs_ctk_src "${_rs_superbuild_root}/CTK")
    if(EXISTS "${_rs_ctk_src}/CMakeLists.txt")
      set(CTK_SOURCE_DIR "${_rs_ctk_src}" CACHE PATH "CTK source tree" FORCE)
    endif()
  endif()
  if(CTK_SOURCE_DIR)
    set(CTK_CMAKE_DIR "${CTK_SOURCE_DIR}/CMake" CACHE PATH "CTK CMake modules" FORCE)
  endif()
  if(CTK_DIR AND EXISTS "${CTK_DIR}/UseCTK.cmake")
    list(APPEND CMAKE_PREFIX_PATH "${CTK_DIR}")
    include("${CTK_DIR}/UseCTK.cmake")
  endif()
  if(CTK_CMAKE_DIR AND EXISTS "${CTK_CMAKE_DIR}/ctkFunctionExtractOptimizedLibrary.cmake")
    list(APPEND CMAKE_MODULE_PATH "${CTK_CMAKE_DIR}")
    include("${CTK_CMAKE_DIR}/ctkFunctionExtractOptimizedLibrary.cmake" OPTIONAL)
  endif()

  # 2) Locate SlicerExecutionModel (optional, but many extensions expect it).
  if(NOT DEFINED SlicerExecutionModel_DIR)
    set(_rs_sem_candidates
      "$ENV{SlicerExecutionModel_DIR}"
      "${_rs_superbuild_root}/SlicerExecutionModel-build"
      "${_rs_superbuild_root}/../SlicerExecutionModel-build"
      "${CMAKE_BINARY_DIR}/../SlicerExecutionModel-build"
      "${CMAKE_BINARY_DIR}/../../SlicerExecutionModel-build"
    )
    foreach(_p IN LISTS _rs_sem_candidates)
      if(_p AND EXISTS "${_p}/SlicerExecutionModelConfig.cmake")
        set(SlicerExecutionModel_DIR "${_p}" CACHE PATH "SEM build tree" FORCE)
        break()
      endif()
    endforeach()
  endif()
  if(NOT DEFINED SlicerExecutionModel_SOURCE_DIR)
    set(_rs_sem_src_candidates
      "$ENV{SlicerExecutionModel_SOURCE_DIR}"
      "${_rs_superbuild_root}/SlicerExecutionModel"
      "${_rs_superbuild_root}/../SlicerExecutionModel"
    )
    foreach(_p IN LISTS _rs_sem_src_candidates)
      if(_p AND EXISTS "${_p}/CMake/SEMMacroBuildCLI.cmake")
        set(SlicerExecutionModel_SOURCE_DIR "${_p}" CACHE PATH "SEM source tree" FORCE)
        break()
      endif()
    endforeach()
  endif()
  if(SlicerExecutionModel_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${SlicerExecutionModel_DIR}")
    if(EXISTS "${SlicerExecutionModel_DIR}/SlicerExecutionModelConfig.cmake")
      include("${SlicerExecutionModel_DIR}/SlicerExecutionModelConfig.cmake")
    endif()
    if(NOT DEFINED SlicerExecutionModel_CLI_LIBRARY_WRAPPER_CXX AND
       SlicerExecutionModel_SOURCE_DIR AND
       EXISTS "${SlicerExecutionModel_SOURCE_DIR}/CMake/SEMCommandLineLibraryWrapper.cxx")
      set(SlicerExecutionModel_CLI_LIBRARY_WRAPPER_CXX
        "${SlicerExecutionModel_SOURCE_DIR}/CMake/SEMCommandLineLibraryWrapper.cxx"
        CACHE FILEPATH "SEM CLI wrapper source file" FORCE)
    endif()
    if(DEFINED SlicerExecutionModel_USE_FILE AND EXISTS "${SlicerExecutionModel_USE_FILE}")
      include("${SlicerExecutionModel_USE_FILE}")
    elseif(SlicerExecutionModel_SOURCE_DIR AND EXISTS "${SlicerExecutionModel_SOURCE_DIR}/CMake/SEMMacroBuildCLI.cmake")
      list(APPEND CMAKE_MODULE_PATH "${SlicerExecutionModel_SOURCE_DIR}/CMake")
      include("${SlicerExecutionModel_SOURCE_DIR}/CMake/SEMMacroBuildCLI.cmake" OPTIONAL)
      include("${SlicerExecutionModel_SOURCE_DIR}/CMake/SEMFunctionAddExecutable.cmake" OPTIONAL)
    endif()
  endif()

  list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
  list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
endif()
]=])
set(_rs_top_level_includes "${CMAKE_PROJECT_TOP_LEVEL_INCLUDES}")
list(APPEND _rs_top_level_includes "${_rs_ext_init}")
list(REMOVE_DUPLICATES _rs_top_level_includes)
set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES "${_rs_top_level_includes}" CACHE STRING "Radiance extension init hook" FORCE)
unset(_rs_top_level_includes)
macro(_bundle_ext name repo tag)
  FetchContent_Declare(${name}
    GIT_REPOSITORY ${repo}
    GIT_TAG        ${tag}
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   1
  )
  FetchContent_GetProperties(${name})
  if(NOT ${name}_POPULATED)
    FetchContent_Populate(${name})
  endif()
  # 直接在当前（调用者）作用域追加
  set(_ext_src_dir "${${name}_SOURCE_DIR}")
  if(NOT _ext_src_dir)
    string(TOLOWER "${name}" _lname)
    set(_ext_src_dir "${CMAKE_BINARY_DIR}/_deps/${_lname}-src")
  endif()
  message(STATUS "Bundle ext: ${name} -> ${_ext_src_dir}")
  list(APPEND Slicer_EXTENSION_SOURCE_DIRS "${_ext_src_dir}")
endmacro()

# 1) Total Segmentator
#    仓库: lassoan/SlicerTotalSegmentator
#    锁定: 2025-09-29 提交 2e5f9c3
_bundle_ext(Ext_TotalSegmentator
  https://github.com/lassoan/SlicerTotalSegmentator.git
  2e5f9c3)

# 2) nnUNet（TotalSegmentator 依赖）
#    仓库: KitwareMedical/SlicerNNUnet
#    锁定: 2025-06-24 提交 e44b008
_bundle_ext(Ext_SlicerNNUnet
  https://github.com/KitwareMedical/SlicerNNUnet.git
  e44b008)

# 3) SegmentEditorExtraEffects（分割额外工具）
#    仓库: lassoan/SlicerSegmentEditorExtraEffects
#    锁定: 2025-09-22 提交 aa3103b
_bundle_ext(Ext_SegEditorExtra
  https://github.com/lassoan/SlicerSegmentEditorExtraEffects.git
  aa3103b)

# 4) Slicer-AirwaySegmentation（气道分割 CLI+模块）
#    仓库: Slicer/SlicerAirwaySegmentation
#    锁定: 2024-06-17 提交 ade2f33
_bundle_ext(Ext_AirwaySeg
  https://github.com/Slicer/SlicerAirwaySegmentation.git
  ade2f33)

# 5) MarkupsToModel（SegmentEditorExtraEffects 依赖）
#    仓库: SlicerIGT/SlicerMarkupsToModel
#    锁定: HEAD at time of integration (312cf9f)
_bundle_ext(Ext_MarkupsToModel
  https://github.com/SlicerIGT/SlicerMarkupsToModel.git
  312cf9f)

# 6) General Registration (Elastix)
#    仓库: lassoan/SlicerElastix
#    分支: master
_bundle_ext(SlicerElastix
  https://github.com/lassoan/SlicerElastix.git
  master)

# 7) Landmark Registration
#    仓库: Slicer/LandmarkRegistration
#    分支: master
_bundle_ext(LandmarkRegistration
  https://github.com/Slicer/LandmarkRegistration.git
  master)

option(RS_ENABLE_BUNDLE_DCM2NII "Bundle SlicerDcm2nii extension (requires compat shim)" OFF)
if(RS_ENABLE_BUNDLE_DCM2NII)
  # 5) SlicerDcm2nii（dcm2niix 前端）
  #    仓库: SlicerDMRI/SlicerDcm2nii
  #    锁定: 2024-04-23 提交 e3551e4
  _bundle_ext(Ext_SlicerDcm2nii
    https://github.com/SlicerDMRI/SlicerDcm2nii.git
    e3551e4)
endif()

list(REMOVE_DUPLICATES Slicer_EXTENSION_SOURCE_DIRS)
if(NOT RS_ENABLE_BUNDLE_DCM2NII)
  list(FILTER Slicer_EXTENSION_SOURCE_DIRS EXCLUDE REGEX "SlicerDcm2nii$|ext_slicerdcm2nii-src$")
endif()
# 清理历史写法遗留的 ext_slicerelastix-src 目录（避免重复注入）
list(FILTER Slicer_EXTENSION_SOURCE_DIRS EXCLUDE REGEX "ext_slicerelastix-src$")
message(STATUS "Bundled extensions: ${Slicer_EXTENSION_SOURCE_DIRS}")

# Ensure bundled extensions gracefully handle empty Slicer_USE_FILE by falling back
# to the minimal shim that provides mark_as_superbuild/ExternalProject helpers.
set(_rs_use_slicer_compat "${CMAKE_CURRENT_LIST_DIR}/UseSlicerCompat.cmake")
file(TO_CMAKE_PATH "${_rs_use_slicer_compat}" _rs_use_slicer_compat)

foreach(_ext_dir IN LISTS Slicer_EXTENSION_SOURCE_DIRS)
  set(_ext_main "${_ext_dir}/CMakeLists.txt")
  if(EXISTS "${_ext_main}")
    file(READ "${_ext_main}" _ext_content)
    set(_ext_patched "${_ext_content}")
    set(_ext_touched FALSE)

    # Track original newline style and normalize to LF for regex handling.
    if(_ext_patched MATCHES "\r\n")
      set(_ext_newline "\r\n")
      string(REPLACE "\r\n" "\n" _ext_patched "${_ext_patched}")
    else()
      set(_ext_newline "\n")
    endif()

    # Ensure EXTENSION_NAME fallback exists (only once).
    if(NOT _ext_patched MATCHES "RS compat] Ensure EXTENSION_NAME")
      set(_name_patch [=[
# [RS compat] Ensure EXTENSION_NAME present
if(NOT DEFINED EXTENSION_NAME)
  set(EXTENSION_NAME ${PROJECT_NAME})
endif()

]=])
      string(REGEX REPLACE
        "([ \t]*[Pp][Rr][Oo][Jj][Ee][Cc][Tt]\\([^\\)]*\\)[ \t]*\n)"
        "\\1${_name_patch}"
        _ext_next
        "${_ext_patched}")
      if(NOT _ext_next STREQUAL _ext_patched)
        set(_ext_patched "${_ext_next}")
        set(_ext_touched TRUE)
      endif()
    endif()

    # Guard include(${Slicer_USE_FILE}) so legacy extensions keep working.
    if(NOT _ext_patched MATCHES "RS compat] Guard include" AND
       _ext_patched MATCHES "include\\([ \t]*\\$\\{Slicer_USE_FILE\\}[ \t]*\\)")
      set(_guard [=[

# [RS compat] Guard include when Slicer_USE_FILE is empty
set(_rs_superbuild_root "")
if(NOT Slicer_USE_FILE AND DEFINED Slicer_DIR)
  set(_rs_usefile_candidates)
  list(APPEND _rs_usefile_candidates "${Slicer_DIR}/UseSlicer.cmake")
  get_filename_component(_rs_rs_parent "${Slicer_DIR}" DIRECTORY)
  if(_rs_rs_parent)
    list(APPEND _rs_usefile_candidates "${_rs_rs_parent}/UseSlicer.cmake")
    get_filename_component(_rs_superbuild_root "${_rs_rs_parent}" DIRECTORY)
    if(_rs_superbuild_root)
      list(APPEND _rs_usefile_candidates "${_rs_superbuild_root}/UseSlicer.cmake")
    endif()
  endif()
  foreach(_rs_candidate IN LISTS _rs_usefile_candidates)
    if(NOT Slicer_USE_FILE AND EXISTS "${_rs_candidate}")
      set(Slicer_USE_FILE "${_rs_candidate}")
    endif()
  endforeach()
endif()
if(NOT _rs_superbuild_root AND DEFINED Slicer_DIR)
  get_filename_component(_rs_superbuild_root "${Slicer_DIR}" DIRECTORY)
  get_filename_component(_rs_superbuild_root "${_rs_superbuild_root}" DIRECTORY)
endif()
if(_rs_superbuild_root)
  set(_rs_ctk_build_dir "${_rs_superbuild_root}/CTK-build")
  if(EXISTS "${_rs_ctk_build_dir}/CTKConfig.cmake")
    set(CTK_DIR "${_rs_ctk_build_dir}" CACHE PATH "CTK build tree" FORCE)
    list(APPEND CMAKE_PREFIX_PATH "${CTK_DIR}")
    list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
  endif()
  set(_rs_ctk_module_dir "${_rs_superbuild_root}/CTK/CMake")
  if(EXISTS "${_rs_ctk_module_dir}")
    list(APPEND CMAKE_MODULE_PATH "${_rs_ctk_module_dir}")
    list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
    set(CTK_CMAKE_DIR "${_rs_ctk_module_dir}")
  endif()
  if(NOT DEFINED SlicerExecutionModel_DIR)
    set(_rs_sem_dir "${_rs_superbuild_root}/SlicerExecutionModel-build")
    if(EXISTS "${_rs_sem_dir}/SlicerExecutionModelConfig.cmake")
      set(SlicerExecutionModel_DIR "${_rs_sem_dir}" CACHE PATH "SEM build tree" FORCE)
    endif()
  endif()
  set(Slicer_SKIP_SlicerBlockAdditionalLauncherSettings ON CACHE BOOL "Skip additional launcher block" FORCE)
endif()
if(Slicer_USE_FILE)
  include(${Slicer_USE_FILE})
elseif(EXISTS "@RS_USE_FILE_COMPAT@")
  include("@RS_USE_FILE_COMPAT@")
endif()
if(DEFINED CTK_DIR AND EXISTS "${CTK_DIR}/UseCTK.cmake")
  include("${CTK_DIR}/UseCTK.cmake")
elseif(DEFINED _rs_ctk_module_dir AND EXISTS "${_rs_ctk_module_dir}/ctkFunctionExtractOptimizedLibrary.cmake")
  include(ctkFunctionExtractOptimizedLibrary)
endif()
unset(_rs_superbuild_root)

]=])
      string(REPLACE "@RS_USE_FILE_COMPAT@" "${_rs_use_slicer_compat}" _guard "${_guard}")
      string(REGEX REPLACE
        "include\\([ \t]*\\$\\{Slicer_USE_FILE\\}[ \t]*\\)"
        "${_guard}"
        _ext_next
        "${_ext_patched}")
      if(NOT _ext_next STREQUAL _ext_patched)
        set(_ext_patched "${_ext_next}")
        set(_ext_touched TRUE)
      endif()
    endif()

    if(_ext_touched)
      if(_ext_newline STREQUAL "\r\n")
        string(REPLACE "\n" "\r\n" _ext_output "${_ext_patched}")
      else()
        set(_ext_output "${_ext_patched}")
      endif()
      file(WRITE "${_ext_main}" "${_ext_output}")
      message(STATUS "Patched extension metadata/guards: ${_ext_dir}")
    endif()
  endif()
endforeach()
