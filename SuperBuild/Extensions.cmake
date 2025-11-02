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
message(STATUS "Bundled extensions: ${Slicer_EXTENSION_SOURCE_DIRS}")

# Workaround for legacy extensions expecting Slicer_USE_FILE in custom-app context.
# If SlicerDcm2nii is present, patch its CMakeLists.txt on-the-fly to avoid failing
# include(${Slicer_USE_FILE}) and to provide mark_as_superbuild() in inner build.
foreach(_ext_dir IN LISTS Slicer_EXTENSION_SOURCE_DIRS)
  if(EXISTS "${_ext_dir}/CMakeLists.txt" AND EXISTS "${_ext_dir}/SuperBuild.cmake")
    file(READ "${_ext_dir}/CMakeLists.txt" _ext_cml)
    if(_ext_cml MATCHES "include\(\$\{Slicer_USE_FILE\}\)")
      set(_shim "\n# [RS compat] Inject shim for Slicer_USE_FILE when empty in custom-app\nif(NOT Slicer_USE_FILE)\n  set(Slicer_USE_FILE \"${CMAKE_CURRENT_LIST_DIR}/UseSlicerCompat.cmake\")\n  file(WRITE \"${CMAKE_CURRENT_LIST_DIR}/UseSlicerCompat.cmake\" \"if(DEFINED Slicer_CMAKE_DIR AND EXISTS \\\"${Slicer_CMAKE_DIR}/ExternalProjectDependency.cmake\\\")\\n  include(\\\"${Slicer_CMAKE_DIR}/ExternalProjectDependency.cmake\\\")\\nendif()\\n\")\nendif()\n")
      string(REPLACE "include(${Slicer_USE_FILE})" "${_shim}include(${Slicer_USE_FILE})" _ext_cml_patched "${_ext_cml}")
      if(NOT _ext_cml STREQUAL _ext_cml_patched)
        file(WRITE "${_ext_dir}/CMakeLists.txt" "${_ext_cml_patched}")
        message(STATUS "Patched extension CMakeLists for compat: ${_ext_dir}")
      endif()
    endif()
  endif()
endforeach()
