from pathlib import Path
from typing import Optional

import qt
import slicer
import vtk
from slicer.ScriptedLoadableModule import (
    ScriptedLoadableModule,
    ScriptedLoadableModuleLogic,
    ScriptedLoadableModuleWidget,
)
from slicer.util import VTKObservationMixin

# Import to ensure the files are available through the Qt resource system
from Resources import HomeResources  # noqa: F401


class Home(ScriptedLoadableModule):
    """The home module allows to orchestrate and style the overall application workflow.

    It is a "special" module in the sense that its role is to customize the application and
    coordinate a workflow between other "regular" modules.

    Associated widget and logic are not intended to be initialized multiple times.
    """

    def __init__(self, parent: Optional[qt.QWidget]):
        ScriptedLoadableModule.__init__(self, parent)
        self.parent.title = "首页"
        self.parent.categories = [""]
        self.parent.dependencies = []
        self.parent.contributors = ["医学影像三维重建软件开发团队"]
        self.parent.helpText = """医学影像三维重建软件首页，提供常用工作流、布局和学习资源。"""
        self.parent.helpText += self.getDefaultModuleDocumentationLink()
        self.parent.acknowledgementText = """本软件基于 3D Slicer 开发（BSD 许可证）。"""


class HomeWidget(ScriptedLoadableModuleWidget, VTKObservationMixin):
    """Uses ScriptedLoadableModuleWidget base class, available at:
    https://github.com/Slicer/Slicer/blob/main/Base/Python/slicer/ScriptedLoadableModule.py
    """

    customLayoutId: int = 558

    @property
    def toolbarNames(self) -> list[str]:
        return [str(k) for k in self._toolbars]

    _toolbars: dict[str, qt.QToolBar] = {}
    _toolbar_icons: dict[str, qt.QIcon] = {}

    def __init__(self, parent: Optional[qt.QWidget]):
        """Called when the application opens the module the first time and the widget is initialized."""
        ScriptedLoadableModuleWidget.__init__(self, parent)
        VTKObservationMixin.__init__(self)

    def setup(self):
        """Called when the application opens the module the first time and the widget is initialized."""
        ScriptedLoadableModuleWidget.setup(self)

        # Load widget from .ui file (created by Qt Designer)
        self.uiWidget = slicer.util.loadUI(self.resourcePath("UI/Home.ui"))
        self.layout.addWidget(self.uiWidget)
        self.ui = slicer.util.childWidgetVariables(self.uiWidget)

        # Get references to relevant underlying modules
        # NA

        # Create logic class
        self.logic = HomeLogic()
        self._toolbar_icons = self._load_toolbar_icons()

        self.customLayoutId = self.logic.register_workspace_layout(
            self.resourcePath("Layouts/RadianceWorkspace.xml")
        )
        self._layoutNode = slicer.app.layoutManager().layoutLogic().GetLayoutNode()
        layoutModifiedEvent = getattr(slicer.vtkMRMLLayoutNode, "LayoutModifiedEvent", vtk.vtkCommand.ModifiedEvent)
        self.addObserver(self._layoutNode, layoutModifiedEvent, self.onLayoutChanged)

        # 避免固定设置局部调色板，以便主题切换时自动继承应用调色板

        self._decorateHomeCards()
        self._configureButtons()
        self.setupQuickActions()
        self.setupLayoutButtons()
        self.setupResourceLinks()

        # Remove unneeded UI elements
        self.modifyWindowUI()
        # Default to showing Slicer UI (menus/toolbars) so users can access Appearance
        self.setCustomUIVisible(False)

        # 纯跟随系统主题：不安装事件过滤，不应用任何模块样式
        self.onLayoutChanged()

    def cleanup(self):
        """Called when the application closes and the module widget is destroyed."""
        self.removeObservers()

    # 不使用事件过滤；完全依赖全局主题的调色板

    def setSlicerUIVisible(self, visible: bool):
        exemptToolbars = [
            "MainToolBar",
            "ViewToolBar",
            *self.toolbarNames,
        ]
        slicer.util.setDataProbeVisible(visible)
        slicer.util.setMenuBarsVisible(visible, ignore=exemptToolbars)
        slicer.util.setModuleHelpSectionVisible(visible)
        slicer.util.setModulePanelTitleVisible(visible)
        slicer.util.setPythonConsoleVisible(visible)
        slicer.util.setApplicationLogoVisible(visible)
        keepToolbars = [slicer.util.findChild(slicer.util.mainWindow(), toolbarName) for toolbarName in exemptToolbars]
        slicer.util.setToolbarsVisible(visible, keepToolbars)

    def modifyWindowUI(self):
        """Customize the entire user interface to resemble the custom application"""
        # Custom toolbars
        self.initializeNavigationToolBar()
        self.initializeSettingsToolBar()

    def insertToolBar(self, beforeToolBarName: str, name: str, title: Optional[str] = None) -> qt.QToolBar:
        """Helper method to insert a new toolbar between existing ones"""
        beforeToolBar = slicer.util.findChild(slicer.util.mainWindow(), beforeToolBarName)

        if title is None:
            title = name

        toolBar = qt.QToolBar(title)
        toolBar.name = name
        slicer.util.mainWindow().insertToolBar(beforeToolBar, toolBar)

        self._toolbars[name] = toolBar

        return toolBar

    def initializeNavigationToolBar(self):
        """Create toolbar hosting shortcut actions for primary workflows"""
        navigationToolBar = self.insertToolBar("MainToolBar", "NavigationToolBar", title="导航")
        navigationToolBar.setToolButtonStyle(qt.Qt.ToolButtonTextUnderIcon)
        navigationToolBar.setIconSize(qt.QSize(36, 36))
        actions = [
            ("首页", self.toolbarIcon("home"), lambda: slicer.util.selectModule("Home")),
            ("DICOM", self.toolbarIcon("dicom"), lambda: slicer.util.selectModule("DICOM")),
            ("数据", self.toolbarIcon("data"), lambda: slicer.util.selectModule("Data")),
            ("分割", self.toolbarIcon("segment"), lambda: slicer.util.selectModule("SegmentEditor")),
            ("渲染", self.toolbarIcon("render"), lambda: slicer.util.selectModule("VolumeRendering")),
        ]

        for text, icon, callback in actions:
            action = navigationToolBar.addAction(icon, text)
            action.triggered.connect(lambda checked=False, fn=callback: fn())  # noqa: B023

    def _decorateHomeCards(self):
        """Apply dynamic properties so QSS can skin the cards and labels"""
        cards = [
            self.ui.quickActionsFrame,
            self.ui.workflowFrame,
            self.ui.layoutFrame,
            self.ui.resourcesFrame,
        ]
        for card in cards:
            self._markAsCard(card)

    def _markAsCard(self, widget: qt.QWidget):
        widget.setProperty("radianceCard", True)
        self._polish(widget)

    def _configureButtons(self):
        """Set button variants and interactions before styling"""
        secondaryButtons = [
            self.ui.loadDicomButton,
            self.ui.volumeRenderingButton,
            self.ui.fourUpLayoutButton,
        ]
        for button in secondaryButtons:
            button.setProperty("variant", "secondary")
            self._polish(button)

    def setupQuickActions(self):
        """Wire quick access buttons to application behavior"""
        self.ui.openDataButton.clicked.connect(self.logic.open_add_data_dialog)
        self.ui.loadDicomButton.clicked.connect(lambda: slicer.util.selectModule("DICOM"))
        self.ui.segmentEditorButton.clicked.connect(lambda: slicer.util.selectModule("SegmentEditor"))
        self.ui.volumeRenderingButton.clicked.connect(lambda: slicer.util.selectModule("VolumeRendering"))

    def setupLayoutButtons(self):
        """Allow switching between custom workspace and classic layouts"""
        self.layoutButtonGroup = qt.QButtonGroup()
        self.layoutButtonGroup.setExclusive(True)
        self.layoutButtonGroup.addButton(self.ui.workspaceLayoutButton)
        self.layoutButtonGroup.addButton(self.ui.fourUpLayoutButton)

        self.ui.workspaceLayoutButton.clicked.connect(
            lambda: self.setLayout(self.customLayoutId)
        )
        self.ui.fourUpLayoutButton.clicked.connect(
            lambda: self.setLayout(slicer.vtkMRMLLayoutNode.SlicerLayoutFourUpView)
        )

    def setupResourceLinks(self):
        """Configure resource panel behavior"""
        self.ui.resourcesLinksLabel.setOpenExternalLinks(True)

    def setLayout(self, layoutId: int):
        layoutLogic = slicer.app.layoutManager().layoutLogic()
        layoutNode = layoutLogic.GetLayoutNode()
        layoutNode.SetLayout(layoutId)

    def onLayoutChanged(self, caller=None, event=None):
        """Sync layout toggle state with current MRML layout"""
        if not hasattr(self, "_layoutNode") or self._layoutNode is None:
            return
        currentLayout = self._layoutNode.GetViewArrangement()
        self.ui.workspaceLayoutButton.setChecked(currentLayout == self.customLayoutId)
        self.ui.fourUpLayoutButton.setChecked(currentLayout == slicer.vtkMRMLLayoutNode.SlicerLayoutFourUpView)

    def initializeSettingsToolBar(self):
        """Create toolbar and dialog for app settings"""
        settingsToolBar = self.insertToolBar("MainToolBar", "SettingsToolBar", title="设置")
        settingsToolBar.setToolButtonStyle(qt.Qt.ToolButtonTextUnderIcon)
        settingsToolBar.setIconSize(qt.QSize(36, 36))

        self.settingsAction = settingsToolBar.addAction(self.toolbarIcon("settings"), "设置")
        self.settingsAction.setToolTip("打开软件设置")

        # Settings dialog
        self.settingsDialog = slicer.util.loadUI(self.resourcePath("UI/Settings.ui"))
        self.settingsUI = slicer.util.childWidgetVariables(self.settingsDialog)
        self.settingsUI.CustomUICheckBox.toggled.connect(self.setCustomUIVisible)
        self.settingsAction.triggered.connect(self.raiseSettings)

    # 移除自定义样式切换，保持最简：始终跟随全局主题

    def raiseSettings(self, _):
        self.settingsDialog.exec()

    def setCustomUIVisible(self, visible: bool):
        self.setSlicerUIVisible(not visible)

    # 不再应用模块级 QSS

    def _polish(self, widget: qt.QWidget):
        widget.style().unpolish(widget)
        widget.style().polish(widget)
        widget.update()

    def _load_toolbar_icons(self) -> dict[str, qt.QIcon]:
        files = {
            "home": "toolbar_home.svg",
            "dicom": "toolbar_dicom.svg",
            "data": "toolbar_data.svg",
            "segment": "toolbar_segment.svg",
            "render": "toolbar_render.svg",
            "settings": "toolbar_settings.svg",
        }
        icons: dict[str, qt.QIcon] = {}
        for key, filename in files.items():
            icons[key] = qt.QIcon(self.resourcePath(f"Icons/Toolbar/{filename}"))
        return icons

    def toolbarIcon(self, name: str) -> qt.QIcon:
        return self._toolbar_icons.get(name, qt.QIcon())

class HomeLogic(ScriptedLoadableModuleLogic):
    """
    Implements underlying logic for the Home module.
    """

    CUSTOM_LAYOUT_ID = 558

    def register_workspace_layout(self, layoutFile: str) -> int:
        """Register custom MRML layout for the Radiance workspace"""
        layoutLogic = slicer.app.layoutManager().layoutLogic()
        layoutNode = layoutLogic.GetLayoutNode()
        description = Path(layoutFile).read_text(encoding="utf-8")
        layoutNode.AddLayoutDescription(self.CUSTOM_LAYOUT_ID, description)
        return self.CUSTOM_LAYOUT_ID

    @staticmethod
    def open_add_data_dialog():
        slicer.util.openAddDataDialog()
