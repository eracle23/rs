from pathlib import Path
from typing import Optional

import qt
import slicer
import vtk
import SlicerCustomAppUtilities
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
        self.parent.title = "Home"
        self.parent.categories = [""]
        self.parent.dependencies = []
        self.parent.contributors = ["Experience Engineering (Radiance Labs)"]
        self.parent.helpText = """Radiance Studio home surfaces common workflows, layouts, and learning resources in one place."""
        self.parent.helpText += self.getDefaultModuleDocumentationLink()
        self.parent.acknowledgementText = """Radiance Studio builds on 3D Slicer (BSD license)."""


class HomeWidget(ScriptedLoadableModuleWidget, VTKObservationMixin):
    """Uses ScriptedLoadableModuleWidget base class, available at:
    https://github.com/Slicer/Slicer/blob/main/Base/Python/slicer/ScriptedLoadableModule.py
    """

    customLayoutId: int = 558

    @property
    def toolbarNames(self) -> list[str]:
        return [str(k) for k in self._toolbars]

    _toolbars: dict[str, qt.QToolBar] = {}

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

        self.customLayoutId = self.logic.register_workspace_layout(
            self.resourcePath("Layouts/RadianceWorkspace.xml")
        )
        self._layoutNode = slicer.app.layoutManager().layoutLogic().GetLayoutNode()
        layoutModifiedEvent = getattr(slicer.vtkMRMLLayoutNode, "LayoutModifiedEvent", vtk.vtkCommand.ModifiedEvent)
        self.addObserver(self._layoutNode, layoutModifiedEvent, self.onLayoutChanged)

        # Dark palette does not propagate on its own
        # See https://github.com/KitwareMedical/SlicerCustomAppTemplate/issues/72
        self.uiWidget.setPalette(slicer.util.mainWindow().style().standardPalette())

        self._decorateHomeCards()
        self._configureButtons()
        self.setupQuickActions()
        self.setupLayoutButtons()
        self.setupResourceLinks()

        # Remove unneeded UI elements
        self.modifyWindowUI()
        self.setCustomUIVisible(True)

        # Apply style
        self.applyApplicationStyle()
        self.onLayoutChanged()

    def cleanup(self):
        """Called when the application closes and the module widget is destroyed."""
        self.removeObservers()

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
        navigationToolBar = self.insertToolBar("MainToolBar", "NavigationToolBar", title="Navigation")
        navigationToolBar.setToolButtonStyle(qt.Qt.ToolButtonTextUnderIcon)
        navigationToolBar.setIconSize(qt.QSize(32, 32))

        style = slicer.util.mainWindow().style()
        actions = [
            ("Home", qt.QIcon(self.resourcePath("Icons/Home.png")), lambda: slicer.util.selectModule("Home")),
            ("Data", style.standardIcon(qt.QStyle.SP_DirOpenIcon), lambda: slicer.util.selectModule("Data")),
            ("Segment", style.standardIcon(qt.QStyle.SP_DialogApplyButton), lambda: slicer.util.selectModule("SegmentEditor")),
            ("Render", style.standardIcon(qt.QStyle.SP_FileDialogDetailedView), lambda: slicer.util.selectModule("VolumeRendering")),
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
        settingsToolBar = self.insertToolBar("MainToolBar", "SettingsToolBar", title="Settings")

        gearIcon = qt.QIcon(self.resourcePath("Icons/Gears.png"))
        self.settingsAction = settingsToolBar.addAction(gearIcon, "")

        # Settings dialog
        self.settingsDialog = slicer.util.loadUI(self.resourcePath("UI/Settings.ui"))
        self.settingsUI = slicer.util.childWidgetVariables(self.settingsDialog)
        self.settingsUI.CustomUICheckBox.toggled.connect(self.setCustomUIVisible)
        self.settingsUI.CustomStyleCheckBox.toggled.connect(self.toggleStyle)
        self.settingsAction.triggered.connect(self.raiseSettings)

    def toggleStyle(self, visible: bool):
        if visible:
            self.applyApplicationStyle()
        else:
            slicer.app.styleSheet = ""

    def raiseSettings(self, _):
        self.settingsDialog.exec()

    def setCustomUIVisible(self, visible: bool):
        self.setSlicerUIVisible(not visible)

    def applyApplicationStyle(self):
        SlicerCustomAppUtilities.applyStyle([slicer.app], self.resourcePath("Home.qss"))
        self.styleThreeDWidget()
        self.styleSliceWidgets()
        self._polish(self.uiWidget)

    def _polish(self, widget: qt.QWidget):
        widget.style().unpolish(widget)
        widget.style().polish(widget)
        widget.update()

    def styleThreeDWidget(self):
        viewNode = slicer.app.layoutManager().threeDWidget(0).mrmlViewNode()  # noqa: F841
        # viewNode.SetBackgroundColor(0.0, 0.0, 0.0)
        # viewNode.SetBackgroundColor2(0.0, 0.0, 0.0)
        # viewNode.SetBoxVisible(False)
        # viewNode.SetAxisLabelsVisible(False)
        # viewNode.SetOrientationMarkerType(slicer.vtkMRMLViewNode.OrientationMarkerTypeAxes)

    def styleSliceWidgets(self):
        for name in slicer.app.layoutManager().sliceViewNames():
            sliceWidget = slicer.app.layoutManager().sliceWidget(name)
            self.styleSliceWidget(sliceWidget)

    def styleSliceWidget(self, sliceWidget: slicer.qMRMLSliceWidget):
        controller = sliceWidget.sliceController()  # noqa: F841
        # controller.sliceViewLabel = ""
        # slicer.util.findChild(sliceWidget, "PinButton").visible = False
        # slicer.util.findChild(sliceWidget, "ViewLabel").visible = False
        # slicer.util.findChild(sliceWidget, "FitToWindowToolButton").visible = False
        # slicer.util.findChild(sliceWidget, "SliceOffsetSlider").spinBoxVisible = False


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
