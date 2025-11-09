#-----------------------------------------------------------------------------
set(MODULE_NAME Home)

#-----------------------------------------------------------------------------
set(MODULE_PYTHON_SCRIPTS
  ${MODULE_NAME}.py
  )

set(MODULE_PYTHON_RESOURCES
  Resources/Icons/${MODULE_NAME}.png
  Resources/Icons/Gears.png
  Resources/Icons/Toolbar/toolbar_home.svg
  Resources/Icons/Toolbar/toolbar_data.svg
  Resources/Icons/Toolbar/toolbar_segment.svg
  Resources/Icons/Toolbar/toolbar_render.svg
  Resources/Icons/Toolbar/toolbar_settings.svg
  Resources/Icons/Toolbar/toolbar_add_data.svg
  Resources/Icons/Toolbar/toolbar_dicom.svg
  Resources/Icons/Toolbar/toolbar_save.svg
  Resources/Icons/Toolbar/toolbar_undo.svg
  Resources/Icons/Toolbar/toolbar_redo.svg
  Resources/Icons/Toolbar/toolbar_snapshot.svg
  Resources/Icons/Toolbar/toolbar_layout.svg
  Resources/Layouts/RadianceWorkspace.xml
  Resources/UI/${MODULE_NAME}.ui
  Resources/UI/Settings.ui
  )

slicerFunctionAddPythonQtResources(MODULE_PYTHON_QRC_RESOURCES
  Resources/${MODULE_NAME}.qrc
  )

#-----------------------------------------------------------------------------
slicerMacroBuildScriptedModule(
  NAME ${MODULE_NAME}
  SCRIPTS ${MODULE_PYTHON_SCRIPTS} ${MODULE_PYTHON_QRC_RESOURCES}
  RESOURCES ${MODULE_PYTHON_RESOURCES}
  )
