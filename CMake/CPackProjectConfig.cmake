# Inject or override NSIS finish page run checkbox at CPack time.
# This file is included by CPack after reading CPackConfig.cmake,
# so it can safely override variables used by the NSIS template.

set(CPACK_NSIS_INSTALLER_MUI_FINISHPAGE_RUN_CODE [=[
!define MUI_FINISHPAGE_RUN "$INSTDIR\bin\AssocPrompt.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Set this for .mrml/.mrb as default"
!define MUI_FINISHPAGE_RUN_NOTCHECKED
]=])

# Remove any existing '!define MUI_FINISHPAGE_RUN' injected earlier into CPACK_NSIS_DEFINES to avoid duplicate defines
if(DEFINED CPACK_NSIS_DEFINES)
  string(REGEX REPLACE "[^\n]*MUI_FINISHPAGE_RUN[^\n]*\n" "" CPACK_NSIS_DEFINES "${CPACK_NSIS_DEFINES}")
endif()

#------------------------------------------------------------------------------
# 中文翻译文件安装
# 将预编译的.qm翻译文件包含到安装包中
#------------------------------------------------------------------------------
# 查找翻译文件目录
set(_translations_qm_dir "")
if(EXISTS "${CPACK_PACKAGE_DIRECTORY}/../translations")
  set(_translations_qm_dir "${CPACK_PACKAGE_DIRECTORY}/../translations")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/../build/translations")
  set(_translations_qm_dir "${CMAKE_SOURCE_DIR}/../build/translations")
endif()

if(_translations_qm_dir AND EXISTS "${_translations_qm_dir}")
  file(GLOB _qm_files "${_translations_qm_dir}/*.qm")
  if(_qm_files)
    message(STATUS "[CPack] Including ${CMAKE_LIST_LENGTH} translation .qm files from: ${_translations_qm_dir}")
    # 这些文件将通过install()规则安装
  endif()
endif()