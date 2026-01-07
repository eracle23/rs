#-----------------------------------------------------------------------------
# ApplySlicerPatches.cmake
# 自动应用 Slicer 源码的本地化 patch
#-----------------------------------------------------------------------------

set(SLICER_PATCHES_DIR "${CMAKE_SOURCE_DIR}/Patches/Slicer")
set(SLICER_PATCH_MARKER "${slicersources_SOURCE_DIR}/.rs_patches_applied")

# 检查是否已经应用过 patch
if(EXISTS "${SLICER_PATCH_MARKER}")
  message(STATUS "Slicer patches already applied, skipping...")
  return()
endif()

# 查找所有 patch 文件
file(GLOB SLICER_PATCH_FILES "${SLICER_PATCHES_DIR}/*.patch")

if(NOT SLICER_PATCH_FILES)
  message(STATUS "No Slicer patches found in ${SLICER_PATCHES_DIR}")
  return()
endif()

# 查找 git 可执行文件
find_program(GIT_EXECUTABLE git)
if(NOT GIT_EXECUTABLE)
  message(WARNING "Git not found, cannot apply patches")
  return()
endif()

# 应用每个 patch
foreach(patch_file ${SLICER_PATCH_FILES})
  get_filename_component(patch_name "${patch_file}" NAME)
  message(STATUS "Applying Slicer patch: ${patch_name}")
  
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" apply --check "${patch_file}"
    WORKING_DIRECTORY "${slicersources_SOURCE_DIR}"
    RESULT_VARIABLE check_result
    OUTPUT_VARIABLE check_output
    ERROR_VARIABLE check_error
  )
  
  if(check_result EQUAL 0)
    # Patch 可以干净地应用
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" apply "${patch_file}"
      WORKING_DIRECTORY "${slicersources_SOURCE_DIR}"
      RESULT_VARIABLE apply_result
      OUTPUT_VARIABLE apply_output
      ERROR_VARIABLE apply_error
    )
    
    if(apply_result EQUAL 0)
      message(STATUS "  Successfully applied: ${patch_name}")
    else()
      message(WARNING "  Failed to apply patch: ${patch_name}")
      message(WARNING "  Error: ${apply_error}")
    endif()
  else()
    # Patch 可能已经部分应用或有冲突
    message(STATUS "  Patch may already be applied or has conflicts: ${patch_name}")
    message(STATUS "  Trying with --3way...")
    
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" apply --3way "${patch_file}"
      WORKING_DIRECTORY "${slicersources_SOURCE_DIR}"
      RESULT_VARIABLE apply_3way_result
      OUTPUT_VARIABLE apply_3way_output
      ERROR_VARIABLE apply_3way_error
    )
    
    if(apply_3way_result EQUAL 0)
      message(STATUS "  Applied with 3-way merge: ${patch_name}")
    else()
      message(STATUS "  Skipping (already applied or incompatible): ${patch_name}")
    endif()
  endif()
endforeach()

# 创建标记文件，避免重复应用
file(WRITE "${SLICER_PATCH_MARKER}" "Patches applied at: ${CMAKE_CURRENT_TIMESTAMP}\n")
foreach(patch_file ${SLICER_PATCH_FILES})
  get_filename_component(patch_name "${patch_file}" NAME)
  file(APPEND "${SLICER_PATCH_MARKER}" "  - ${patch_name}\n")
endforeach()

message(STATUS "Slicer patches applied successfully")

