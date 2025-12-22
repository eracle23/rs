#============================================================================
#
# Copyright (c) Kitware, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#============================================================================

project(RadianceApp)

# MSVC UTF-8 源文件编码支持（包含中文字符）
if(MSVC)
  add_compile_options(/utf-8)
endif()

include(SlicerMacroBuildApplication)

# --------------------------------------------------------------------------
# Properties
# --------------------------------------------------------------------------
SlicerReadApplicationProperties()

# --------------------------------------------------------------------------
# Folder
# --------------------------------------------------------------------------
set(${PROJECT_NAME}_FOLDER "App-${PROJECT_NAME}")

# --------------------------------------------------------------------------
# Application library
# --------------------------------------------------------------------------
set(APPLIB_NAME "q${PROJECT_NAME}")

set(APPLIB_SRCS
  qRadianceAppMainWindow.cxx
  qRadianceAppMainWindow.h
  Widgets/ThemeSync.cxx
  Widgets/ThemeSync.h
  Widgets/qAppStyle.cxx
  Widgets/qAppStyle.h
  )

set(APPLIB_MOC_SRCS
  qRadianceAppMainWindow.h
  Widgets/ThemeSync.h
  Widgets/qAppStyle.h
  )

set(APPLIB_UI_SRCS
  )

set(APPLIB_RESOURCES
  Resources/App.qrc
  Resources/ToolbarIcons.qrc
  )

slicerMacroBuildAppLibrary(
  NAME ${APPLIB_NAME}
  DESCRIPTION_SUMMARY ${${PROJECT_NAME}_DESCRIPTION_SUMMARY}
  DESCRIPTION_FILE ${${PROJECT_NAME}_DESCRIPTION_FILE}
  APPLICATION_NAME ${${PROJECT_NAME}_APPLICATION_NAME}
  EXPORT_DIRECTIVE "Q_RADIANCE_APP_EXPORT"
  FOLDER ${${PROJECT_NAME}_FOLDER}
  SRCS ${APPLIB_SRCS}
  MOC_SRCS ${APPLIB_MOC_SRCS}
  UI_SRCS ${APPLIB_UI_SRCS}
  RESOURCES ${APPLIB_RESOURCES}
  )

# --------------------------------------------------------------------------
# Application executable
# --------------------------------------------------------------------------

# Configure launcher only for the main application
set(extra_args)
if(${PROJECT_NAME} STREQUAL ${Slicer_MAIN_PROJECT})
  set(extra_args CONFIGURE_LAUNCHER)
endif()

set(APP_SRCS
  Main.cxx
  )

set(APP_TARGET_LIBRARIES ${APPLIB_NAME})

slicerMacroBuildApplication(
  NAME ${PROJECT_NAME}
  APPLICATION_NAME ${${PROJECT_NAME}_APPLICATION_NAME}
  LAUNCHER_SPLASHSCREEN_FILE ${${PROJECT_NAME}_LAUNCHER_SPLASHSCREEN_FILE}
  APPLE_ICON_FILE ${${PROJECT_NAME}_APPLE_ICON_FILE}
  WIN_ICON_FILE ${${PROJECT_NAME}_WIN_ICON_FILE}
  LICENSE_FILE ${${PROJECT_NAME}_LICENSE_FILE}
  FOLDER ${${PROJECT_NAME}_FOLDER}
  SRCS ${APP_SRCS}
  TARGET_LIBRARIES ${APP_TARGET_LIBRARIES}
  TARGET_NAME_VAR "APP_TARGET_NAME"
  DEFAULT_SETTINGS_FILE Resources/Settings/DefaultSettings.ini
  ${extra_args}
  )

# Ensure the GUI launcher with patched icon (AliceW.exe) is always generated
# before we attempt to install/package it. Otherwise CPack may fail with
# “file INSTALL cannot find …/CMakeFiles/AliceW.exe”.
if(WIN32)
  set(_launcher_artifacts_target "${SLICERAPP_APPLICATION_NAME}LauncherArtifacts")
  if(TARGET ${_launcher_artifacts_target})
    # already defined elsewhere (unlikely), keep existing definition
  else()
    add_custom_target(${_launcher_artifacts_target} ALL)
    set_target_properties(${_launcher_artifacts_target} PROPERTIES
      FOLDER ${${PROJECT_NAME}_FOLDER})
  endif()

  if(TARGET ${SLICERAPP_APPLICATION_NAME}UpdateLauncherWIcon)
    add_dependencies(${_launcher_artifacts_target} ${SLICERAPP_APPLICATION_NAME}UpdateLauncherWIcon)
  elseif(TARGET UpdateLauncherWIcon) # fallback for legacy naming
    add_dependencies(${_launcher_artifacts_target} UpdateLauncherWIcon)
  else()
    message(WARNING
      "No launcher icon patch target found (expected ${SLICERAPP_APPLICATION_NAME}UpdateLauncherWIcon). "
      "Installer may contain unpatched launcher icons.")
  endif()

  if(TARGET package)
    add_dependencies(package ${_launcher_artifacts_target})
  endif()
endif()

# Ensure packaged installs ship scripted modules by injecting launcher-side search paths.
set(RS_LAUNCHER_ENV_OVERLAY_TEMPLATE
  "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Settings/AliceLauncherSettings.scripted-modules.ini.in")
set(RS_LAUNCHER_ENV_OVERLAY "${CMAKE_CURRENT_BINARY_DIR}/AliceLauncherSettings.scripted-modules.ini")
set(RS_SLICER_MM "${Slicer_VERSION_MAJOR}.${Slicer_VERSION_MINOR}")
set(RS_APP_NAME "${SLICERAPP_APPLICATION_NAME}")
configure_file(
  ${RS_LAUNCHER_ENV_OVERLAY_TEMPLATE}
  ${RS_LAUNCHER_ENV_OVERLAY}
  @ONLY
  )
file(TO_CMAKE_PATH "${RS_LAUNCHER_ENV_OVERLAY}" RS_LAUNCHER_ENV_OVERLAY_CMAKE)
install(CODE
  "
  set(_launcher_ini \"\${CMAKE_INSTALL_PREFIX}/${Slicer_INSTALL_BIN_DIR}/${SLICERAPP_APPLICATION_NAME}LauncherSettings.ini\")
  if(EXISTS \"\${_launcher_ini}\")
    file(READ \"\${_launcher_ini}\" _existing_launcher_ini)
    string(FIND \"\${_existing_launcher_ini}\" \"SLICER_QTSCRIPTEDMODULES_PATHS\" _launcher_env_match)
    if(_launcher_env_match EQUAL -1)
      file(READ \"${RS_LAUNCHER_ENV_OVERLAY_CMAKE}\" _overlay_content)
      file(APPEND \"\${_launcher_ini}\" \"\\n\${_overlay_content}\\n\")
      message(STATUS \"${SLICERAPP_APPLICATION_NAME} launcher updated with scripted module search paths: \${_launcher_ini}\")
    else()
      message(STATUS \"${SLICERAPP_APPLICATION_NAME} launcher already defines SLICER_QTSCRIPTEDMODULES_PATHS: \${_launcher_ini}\")
    endif()
  else()
    message(WARNING \"Launcher settings missing, cannot append scripted module paths: \${_launcher_ini}\")
  endif()
  "
  COMPONENT Runtime
  )

# Build and install Windows association helper (per-user, default apps UI launcher)
add_subdirectory(Utilities/AssocPrompt)

# --------------------------------------------------------------------------
# SlicerLanguageTranslations - 集成中文翻译
# --------------------------------------------------------------------------
# CMAKE_CURRENT_SOURCE_DIR = Applications/RadianceApp, 回退到项目根目录
get_filename_component(RS_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(SLICER_TRANSLATIONS_DIR "${RS_PROJECT_ROOT}/SlicerLanguageTranslations-main/translations")

if(Slicer_BUILD_I18N_SUPPORT AND EXISTS "${SLICER_TRANSLATIONS_DIR}")
  find_package(Qt5 COMPONENTS LinguistTools QUIET)
  if(Qt5LinguistTools_FOUND)
    message(STATUS "Building SlicerLanguageTranslations for zh-CN...")
    
    # 收集中文翻译 .ts 文件
    file(GLOB ZH_CN_TS_FILES
      "${SLICER_TRANSLATIONS_DIR}/Slicer_zh-CN.ts"
      "${SLICER_TRANSLATIONS_DIR}/CTK_zh-CN.ts"
      "${SLICER_TRANSLATIONS_DIR}/LanguagePacks_zh-CN.ts"
    )
    
    set(QM_OUTPUT_DIR "${CMAKE_BINARY_DIR}/${Slicer_QM_DIR}")
    file(MAKE_DIRECTORY "${QM_OUTPUT_DIR}")
    
    set(ZH_CN_QM_FILES)
    foreach(ts_file ${ZH_CN_TS_FILES})
      get_filename_component(ts_name "${ts_file}" NAME_WE)
      set(qm_file "${QM_OUTPUT_DIR}/${ts_name}.qm")
      
      add_custom_command(
        OUTPUT "${qm_file}"
        COMMAND Qt5::lrelease "${ts_file}" -qm "${qm_file}"
        DEPENDS "${ts_file}"
        COMMENT "Generating ${ts_name}.qm from .ts"
        VERBATIM
      )
      list(APPEND ZH_CN_QM_FILES "${qm_file}")
    endforeach()
    
    add_custom_target(SlicerTranslations_zh_CN ALL
      DEPENDS ${ZH_CN_QM_FILES}
      COMMENT "Building Chinese translations"
    )
    
    # 安装 .qm 文件
    install(FILES ${ZH_CN_QM_FILES}
      DESTINATION "${Slicer_INSTALL_QM_DIR}"
      COMPONENT Runtime
    )
    
    message(STATUS "Chinese translations configured: ${ZH_CN_TS_FILES}")
  else()
    message(WARNING "Qt5 LinguistTools not found, skipping translations")
  endif()
else()
  if(NOT Slicer_BUILD_I18N_SUPPORT)
    message(STATUS "I18N support disabled, skipping translations")
  elseif(NOT EXISTS "${SLICER_TRANSLATIONS_DIR}")
    message(WARNING "SlicerLanguageTranslations not found at ${SLICER_TRANSLATIONS_DIR}")
  endif()
endif()
