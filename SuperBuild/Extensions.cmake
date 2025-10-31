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

function(_bundle_ext name repo tag)
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
  # 追加到扩展源码路径列表（供打包使用）
  set(Slicer_EXTENSION_SOURCE_DIRS
      "${Slicer_EXTENSION_SOURCE_DIRS};${${name}_SOURCE_DIR}"
      PARENT_SCOPE)
endfunction()

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

# 5) SlicerDcm2nii（dcm2niix 前端）
#    仓库: SlicerDMRI/SlicerDcm2nii
#    锁定: 2024-04-23 提交 e3551e4
_bundle_ext(Ext_SlicerDcm2nii
  https://github.com/SlicerDMRI/SlicerDcm2nii.git
  e3551e4)

list(REMOVE_DUPLICATES Slicer_EXTENSION_SOURCE_DIRS)
message(STATUS "Bundled extensions: ${Slicer_EXTENSION_SOURCE_DIRS}")
