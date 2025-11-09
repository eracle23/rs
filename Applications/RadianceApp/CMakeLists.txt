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
