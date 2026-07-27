cmake_minimum_required(VERSION 3.16.3)

#-----------------------------------------------------------------------------
# 当作为 Slicer Applications 子目录被 add_subdirectory 添加时，执行应用库构建
#-----------------------------------------------------------------------------
get_filename_component(_radiance_parent ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
if(DEFINED Slicer_APPLICATIONS_DIR AND _radiance_parent STREQUAL Slicer_APPLICATIONS_DIR)
  # 内联应用构建（老版本 RadianceApp 单文件风格，无 CMakeLists.AppLib.cmake）
  project(RadianceApp)

  if(MSVC)
    add_compile_options(/utf-8)
  endif()

  include(SlicerMacroBuildApplication)
  SlicerReadApplicationProperties()

  # SlicerReadApplicationProperties 不导出 VERSION_RELEASE / VERSION_TWEAK，需本地再读属性文件
  include(${CMAKE_CURRENT_SOURCE_DIR}/slicer-application-properties.cmake)
  if(NOT DEFINED VERSION_RELEASE OR VERSION_RELEASE STREQUAL "")
    set(VERSION_RELEASE "V${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}")
  endif()

  set(${PROJECT_NAME}_FOLDER "App-${RadianceApp_APPLICATION_NAME}")

  set(APPLIB_NAME "q${PROJECT_NAME}")
  find_package(Qt5 COMPONENTS Sql QUIET)
  if(NOT Qt5Sql_FOUND)
    message(FATAL_ERROR "Qt5 Sql is required for user login (UserManager).")
  endif()

  set(APPLIB_SRCS
    qRadianceAppMainWindow.cxx
    qRadianceAppMainWindow.h
    qRadianceAppMainWindow_p.h
    BrandingPreferences.h
    Widgets/AppLogger.cxx
    Widgets/AppLogger.h
    Widgets/ThemeSync.cxx
    Widgets/ThemeSync.h
    Widgets/SystemColorScheme.cxx
    Widgets/SystemColorScheme.h
    Widgets/SystemColorSchemeWatcher.cxx
    Widgets/SystemColorSchemeWatcher.h
    Widgets/qAppStyle.cxx
    Widgets/qAppStyle.h
    Widgets/qLoginDialog.cxx
    Widgets/qLoginDialog.h
    Widgets/UserManager.cxx
    Widgets/UserManager.h
    Widgets/qUserManagementDialog.cxx
    Widgets/qUserManagementDialog.h
    Widgets/qChangePasswordDialog.cxx
    Widgets/qChangePasswordDialog.h
    Widgets/LicenseManager.cxx
    Widgets/LicenseManager.h
    Widgets/DoctorAnnotationWidget.cxx
    Widgets/DoctorAnnotationWidget.h
    Widgets/VolumeImportValidator.cxx
    Widgets/VolumeImportValidator.h
    Widgets/RadianceShellCleaner.cxx
    Widgets/RadianceShellCleaner.h
    Widgets/SliceColorAdapter.cxx
    Widgets/SliceColorAdapter.h
  )
  set(APPLIB_MOC_SRCS
    qRadianceAppMainWindow.h
    Widgets/AppLogger.h
    Widgets/ThemeSync.h
    Widgets/SystemColorSchemeWatcher.h
    Widgets/qAppStyle.h
    Widgets/qLoginDialog.h
    Widgets/UserManager.h
    Widgets/qUserManagementDialog.h
    Widgets/qChangePasswordDialog.h
    Widgets/LicenseManager.h
    Widgets/DoctorAnnotationWidget.h
    Widgets/SliceColorAdapter.h
  )
  set(APPLIB_UI_SRCS)
  set(APPLIB_RESOURCES
    Resources/App.qrc
    Resources/ToolbarIcons.qrc
    ${qSlicerBaseQTGUI_SOURCE_DIR}/Resources/qSlicerBaseQTGUI.qrc
  )

  configure_file(
    Widgets/RadianceAppVersion.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/RadianceAppVersion.h
    @ONLY
    )

  slicerMacroBuildAppLibrary(
    NAME ${APPLIB_NAME}
    APPLICATION_NAME ${RadianceApp_APPLICATION_NAME}
    DESCRIPTION_SUMMARY ${RadianceApp_DESCRIPTION_SUMMARY}
    DESCRIPTION_FILE ${RadianceApp_DESCRIPTION_FILE}
    EXPORT_DIRECTIVE "Q_RADIANCE_APP_EXPORT"
    FOLDER ${${PROJECT_NAME}_FOLDER}
    SRCS ${APPLIB_SRCS}
    MOC_SRCS ${APPLIB_MOC_SRCS}
    UI_SRCS ${APPLIB_UI_SRCS}
    RESOURCES ${APPLIB_RESOURCES}
    TARGET_LIBRARIES Qt5::Sql
  )

  if(TARGET ${APPLIB_NAME})
    target_include_directories(${APPLIB_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  endif()

  set(extra_args)
  if(${PROJECT_NAME} STREQUAL ${Slicer_MAIN_PROJECT})
    set(extra_args CONFIGURE_LAUNCHER)
  endif()

  set(APP_SRCS Main.cxx)
  slicerMacroBuildApplication(
    NAME ${PROJECT_NAME}
    APPLICATION_NAME ${${PROJECT_NAME}_APPLICATION_NAME}
    LAUNCHER_SPLASHSCREEN_FILE ${RadianceApp_LAUNCHER_SPLASHSCREEN_FILE}
    APPLE_ICON_FILE ${RadianceApp_APPLE_ICON_FILE}
    WIN_ICON_FILE ${RadianceApp_WIN_ICON_FILE}
    LICENSE_FILE ${RadianceApp_LICENSE_FILE}
    DEFAULT_SETTINGS_FILE Resources/Settings/DefaultSettings.ini
    FOLDER ${${PROJECT_NAME}_FOLDER}
    SRCS ${APP_SRCS}
    TARGET_LIBRARIES ${APPLIB_NAME}
    TARGET_NAME_VAR "APP_TARGET_NAME"
    ${extra_args}
  )

  if(TARGET ${APP_TARGET_NAME})
    target_include_directories(${APP_TARGET_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  endif()

  # Ensure the GUI launcher with patched icon (VisionMagicEcosystemW.exe) is always generated
  # before we attempt to install/package it. Otherwise CPack may fail with
  # "file INSTALL cannot find …/CMakeFiles/VisionMagicEcosystemW.exe".
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
    elseif(TARGET UpdateLauncherWIcon)
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

  # 安装 .slicerrc.py 到 lib/AppName-version/Settings/（安装目录，执行 INSTALL 时生效）
  set(RS_SETTINGS_INSTALL_DIR "lib/${SLICERAPP_APPLICATION_NAME}-${Slicer_VERSION_MAJOR}.${Slicer_VERSION_MINOR}/Settings")
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Settings/.slicerrc.py"
    DESTINATION "${RS_SETTINGS_INSTALL_DIR}"
    COMPONENT Runtime
    )

  # 构建目录也生成 Settings/.slicerrc.py；launcher 从 exe 所在 bin/ 找 ../lib/.../Settings/.slicerrc.py，
  # 故必须放到「Slicer 构建根」下的 lib/...。若在子项目中执行，CMAKE_BINARY_DIR 会指向子项目目录，需用 Slicer_BINARY_DIR。
  if(DEFINED Slicer_BINARY_DIR)
    set(RS_LIB_SETTINGS_BUILD_DIR "${Slicer_BINARY_DIR}/lib/${SLICERAPP_APPLICATION_NAME}-${Slicer_VERSION_MAJOR}.${Slicer_VERSION_MINOR}/Settings")
  else()
    set(RS_LIB_SETTINGS_BUILD_DIR "${CMAKE_BINARY_DIR}/lib/${SLICERAPP_APPLICATION_NAME}-${Slicer_VERSION_MAJOR}.${Slicer_VERSION_MINOR}/Settings")
  endif()
  file(MAKE_DIRECTORY "${RS_LIB_SETTINGS_BUILD_DIR}")
  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Settings/.slicerrc.py" DESTINATION "${RS_LIB_SETTINGS_BUILD_DIR}")

  # Build and install Windows association helper (per-user, default apps UI launcher)
  add_subdirectory(Utilities/AssocPrompt)

  # --------------------------------------------------------------------------
  # SlicerLanguageTranslations - 集成中文翻译（老版本 RadianceApp 风格）
  # --------------------------------------------------------------------------
  get_filename_component(RS_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
  set(SLICER_TRANSLATIONS_DIR "${RS_PROJECT_ROOT}/SlicerLanguageTranslations-main/translations")

  if(Slicer_BUILD_I18N_SUPPORT AND EXISTS "${SLICER_TRANSLATIONS_DIR}")
    find_package(Qt5 COMPONENTS LinguistTools QUIET)
    if(Qt5LinguistTools_FOUND)
      message(STATUS "Building SlicerLanguageTranslations for zh-CN...")

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

      set_property(GLOBAL APPEND PROPERTY Slicer_QM_OUTPUT_DIRS "${QM_OUTPUT_DIR}")

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

  return()
endif()
unset(_radiance_parent)

# 未作为 Slicer Applications 子目录时，提示使用顶层 rs/CMakeLists.txt
message(FATAL_ERROR
  "RadianceApp 必须通过 Slicer 内层构建调用。\n"
  "请使用项目根目录 rs/ 作为源码路径配置：cmake -S rs -B build")
