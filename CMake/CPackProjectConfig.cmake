# Inject or override NSIS finish page run checkbox at CPack time.
# This file is included by CPack after reading CPackConfig.cmake,
# so it can safely override variables used by the NSIS template.

set(CPACK_NSIS_INSTALLER_MUI_FINISHPAGE_RUN_CODE [=[
!define MUI_FINISHPAGE_RUN "$INSTDIR\bin\AssocPrompt.exe"
!define MUI_FINISHPAGE_RUN_TEXT "将 *.mrml/*.mrb 设为默认由 ${CPACK_PACKAGE_NAME} 打开"
!define MUI_FINISHPAGE_RUN_NOTCHECKED
]=])

# Remove any existing '!define MUI_FINISHPAGE_RUN' injected earlier into CPACK_NSIS_DEFINES to avoid duplicate defines
if(DEFINED CPACK_NSIS_DEFINES)
  string(REGEX REPLACE "[^\n]*MUI_FINISHPAGE_RUN[^\n]*\n" "" CPACK_NSIS_DEFINES "${CPACK_NSIS_DEFINES}")
endif()
