# Usage:
#   cmake -D PY_ROOT="C:/W/Slicer-build" -P Tools/EnsurePython3LibAlias.cmake
# or
#   cmake -D PYTHON_LIB_DIRS="C:/W/Slicer-build/python-build/CMakeBuild/libpython/Release;C:/W/Slicer-build/python-build/libs;C:/W/Slicer-build/python-install/libs" -P Tools/EnsurePython3LibAlias.cmake

if(NOT DEFINED PYTHON_LIB_DIRS)
  if(NOT DEFINED PY_ROOT)
    message(FATAL_ERROR "Set PY_ROOT (e.g. C:/W/Slicer-build) or provide explicit PYTHON_LIB_DIRS list")
  endif()
  file(TO_CMAKE_PATH "${PY_ROOT}/python-build/CMakeBuild/libpython/Release" _d1)
  file(TO_CMAKE_PATH "${PY_ROOT}/python-build/libs" _d2)
  file(TO_CMAKE_PATH "${PY_ROOT}/python-install/libs" _d3)
  set(PYTHON_LIB_DIRS "${_d1};${_d2};${_d3}")
endif()

# Python version component used by CPython libs (e.g. python312.lib)
if(NOT DEFINED PYVER)
  set(PYVER 312)
endif()

set(_made_any FALSE)

function(_ensure_alias _dir _src _dst)
  if(NOT EXISTS "${_dir}")
    return()
  endif()
  set(_src_path "${_dir}/${_src}")
  set(_dst_path "${_dir}/${_dst}")
  if(EXISTS "${_src_path}" AND NOT EXISTS "${_dst_path}")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_src_path}" "${_dst_path}"
                    RESULT_VARIABLE _copy_res)
    if(_copy_res EQUAL 0)
      message(STATUS "Created alias: ${_dst_path} -> ${_src}")
      set(_made_any TRUE PARENT_SCOPE)
    else()
      message(WARNING "Failed to create alias: ${_dst_path} -> ${_src}")
    endif()
  endif()
endfunction()

foreach(_d IN LISTS PYTHON_LIB_DIRS)
  # Release
  _ensure_alias("${_d}" "python${PYVER}.lib"    "python3.lib")
  # Debug
  _ensure_alias("${_d}" "python${PYVER}_d.lib"  "python3_d.lib")
endforeach()

if(NOT _made_any)
  message(STATUS "No python3.lib alias created (either already present or source libs not found). Dirs: ${PYTHON_LIB_DIRS}")
endif()

