#------------------------------------------------------------------------------
# SlicerExternalTranslations.cmake
#
# 编译外部翻译文件（如SlicerLanguageTranslations）为.qm文件并安装到Slicer
#
# 使用方法（在内层Slicer构建中调用）：
#   include(SlicerExternalTranslations)
#   slicer_add_external_translations(
#     TRANSLATIONS_DIR "${CMAKE_SOURCE_DIR}/../SlicerLanguageTranslations-main/translations"
#     LANGUAGES "zh-CN"
#     COMPONENTS Slicer CTK
#   )
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# slicer_add_external_translations()
#
# 编译外部翻译文件并安装
#------------------------------------------------------------------------------
function(slicer_add_external_translations)
  set(options)
  set(oneValueArgs TRANSLATIONS_DIR)
  set(multiValueArgs LANGUAGES COMPONENTS)
  cmake_parse_arguments(MY "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT Slicer_BUILD_I18N_SUPPORT)
    message(STATUS "[ExternalTranslations] I18N support disabled, skipping")
    return()
  endif()

  if(NOT MY_TRANSLATIONS_DIR)
    message(WARNING "[ExternalTranslations] TRANSLATIONS_DIR not specified")
    return()
  endif()

  if(NOT EXISTS "${MY_TRANSLATIONS_DIR}")
    message(WARNING "[ExternalTranslations] Translations directory not found: ${MY_TRANSLATIONS_DIR}")
    return()
  endif()

  if(NOT MY_LANGUAGES)
    set(MY_LANGUAGES "zh-CN")
  endif()

  if(NOT MY_COMPONENTS)
    set(MY_COMPONENTS Slicer CTK)
  endif()

  # 收集所有需要编译的.ts文件
  set(ALL_TS_FILES)
  set(FOUND_INFO)

  foreach(lang ${MY_LANGUAGES})
    # 尝试多种语言代码格式
    string(REPLACE "-" "_" lang_underscore "${lang}")
    string(REPLACE "_" "-" lang_hyphen "${lang}")

    set(lang_variants "${lang}" "${lang_underscore}" "${lang_hyphen}")
    list(REMOVE_DUPLICATES lang_variants)

    foreach(component ${MY_COMPONENTS})
      set(_found FALSE)
      foreach(lang_var ${lang_variants})
        set(ts_file "${MY_TRANSLATIONS_DIR}/${component}_${lang_var}.ts")
        if(EXISTS "${ts_file}" AND NOT _found)
          list(APPEND ALL_TS_FILES "${ts_file}")
          list(APPEND FOUND_INFO "${component}[${lang_var}]")
          set(_found TRUE)
        endif()
      endforeach()
    endforeach()
  endforeach()

  if(NOT ALL_TS_FILES)
    message(WARNING "[ExternalTranslations] No translation files found")
    return()
  endif()

  list(REMOVE_DUPLICATES ALL_TS_FILES)
  list(LENGTH ALL_TS_FILES _ts_count)
  message(STATUS "[ExternalTranslations] Found ${_ts_count} translation files: ${FOUND_INFO}")

  # 使用Qt的lrelease编译.ts为.qm
  find_package(Qt5 COMPONENTS LinguistTools QUIET)

  if(Qt5LinguistTools_FOUND)
    # 使用qt5_add_translation编译翻译文件
    set(CMAKE_AUTORCC OFF)  # 避免与翻译编译冲突

    # qt5_add_translation会生成.qm文件到CMAKE_CURRENT_BINARY_DIR
    qt5_add_translation(QM_FILES ${ALL_TS_FILES})

    # 创建目标
    add_custom_target(SlicerExternalTranslations ALL DEPENDS ${QM_FILES})

    # 安装到translations目录
    if(DEFINED Slicer_INSTALL_QM_DIR)
      set(_install_dest "${Slicer_INSTALL_QM_DIR}")
    else()
      set(_install_dest "bin/translations")
    endif()

    install(FILES ${QM_FILES}
      DESTINATION "${_install_dest}"
      COMPONENT Runtime
    )

    message(STATUS "[ExternalTranslations] Will compile ${_ts_count} files and install to: ${_install_dest}")

    # 追加到全局QM输出目录属性
    get_filename_component(_qm_output_dir "${CMAKE_CURRENT_BINARY_DIR}" ABSOLUTE)
    set_property(GLOBAL APPEND PROPERTY Slicer_QM_OUTPUT_DIRS "${_qm_output_dir}")

    # 导出结果
    set(SLICER_EXTERNAL_QM_FILES ${QM_FILES} PARENT_SCOPE)

  else()
    message(WARNING "[ExternalTranslations] Qt5LinguistTools not found - translations will not be compiled")
    message(STATUS "[ExternalTranslations] Hint: Ensure Qt5 LinguistTools is available in Qt installation")
  endif()

endfunction()

