# -*- coding: utf-8 -*-
"""
VisionMagic 启动配置脚本
在 Slicer 启动时自动执行，用于配置 UI 精简和功能限制。

【如何确认脚本是否被调用】
1. 启动应用后，检查 %USERPROFILE%\\.visionmagic\\slicerrc_loaded.txt 是否存在且时间戳为最近
2. 若存在且时间戳为新，说明脚本已加载；文件内容含 SLICERRC 路径
3. 从命令行启动时，若控制台可见，会看到：VisionMagic: .slicerrc.py 已加载 (SLICERRC=...)
4. 若 marker 文件不存在或时间戳很旧，说明脚本未被调用，请检查：
   - 安装目录下是否存在 lib/VisionMagicEcosystem-5.8/Settings/.slicerrc.py
   - LauncherSettings.ini 中是否有 SLICERRC=... 且路径正确（launcher 会先设置环境变量再启动 exe）
5. 调试时设置环境变量 VISIONMAGIC_DEBUG=1：保留 Error Log、写入详细 marker 排查日志
6. 默认（未设 VISIONMAGIC_DEBUG）仅把错误写入 marker，不记录每次 UI 隐藏成功的流水日志
"""

import os
import sys
from datetime import datetime

# 最先打印，用于确认本脚本是否被 Slicer 调用（从命令行启动时可看到）
_slicerrc_path = os.environ.get("SLICERRC", "")
print(f"VisionMagic: .slicerrc.py 已加载 (SLICERRC={_slicerrc_path})", flush=True)

# 写入 marker 文件以确认脚本是否被执行（GUI 模式下 console 不可见时使用）
_marker_dir = os.path.join(os.path.expanduser("~"), ".visionmagic")
_marker_file = os.path.join(_marker_dir, "slicerrc_loaded.txt")

# 调试模式：VISIONMAGIC_DEBUG=1 时保留 Error Log 可见，便于查看 print 输出
_VISIONMAGIC_DEBUG = os.environ.get("VISIONMAGIC_DEBUG", "") == "1"


def _appendMarkerLog(msg):
    """追加日志到 marker 文件（错误等，始终记录）。"""
    try:
        os.makedirs(_marker_dir, exist_ok=True)
        with open(_marker_file, "a", encoding="utf-8") as f:
            f.write("[%s] %s\n" % (datetime.now().isoformat(), msg))
    except Exception:
        pass


def _markerLogDebug(msg):
    """详细排查日志，仅 VISIONMAGIC_DEBUG=1 时写入 marker。"""
    if _VISIONMAGIC_DEBUG:
        _appendMarkerLog(msg)


try:
    os.makedirs(_marker_dir, exist_ok=True)
    with open(_marker_file, "w", encoding="utf-8") as f:
        f.write("loaded at %s\nSLICERRC=%s\n" % (datetime.now().isoformat(), _slicerrc_path))
except Exception:
    pass

# 将 Settings 目录加入 Python 路径（由 SLICERRC 或 VISIONMAGIC_SETTINGS_DIR 配置）
_settings_dir = os.environ.get("VISIONMAGIC_SETTINGS_DIR", "")
if not _settings_dir and _slicerrc_path:
    _settings_dir = os.path.dirname(os.path.abspath(_slicerrc_path))
if _settings_dir and _settings_dir not in sys.path:
    sys.path.insert(0, _settings_dir)

import slicer
import qt
_markerLogDebug("import slicer, qt ok")

# 允许的效果列表（与下方配置逻辑共用）
_ALLOWED_EFFECTS = [
    "Threshold",
    "Paint",
    "Erase",
    "Scissors",
    "Margin",
    "Smoothing",
    "Islands",
    "Logical operators",
]


def _is_segment_editor_module(name):
    """判断是否为分割编辑器或分割模块（Segment Editor 可能嵌入在 Segmentations 中）。"""
    if not name:
        return False
    n = str(name).strip()
    return n in ("SegmentEditor", "Segment Editor", "Segmentations")


def configureSegmentEditorEffects():
    """
    配置 Segment Editor 只显示需要的效果工具。
    查找应用中所有 qMRMLSegmentEditorWidget 并应用配置。
    """
    try:
        settings = qt.QSettings()
        settings.setValue("SegmentEditor/EffectNameOrder", _ALLOWED_EFFECTS)
        settings.setValue("SegmentEditor/UnorderedEffectsVisible", False)

        count = 0

        def apply_to_widget(editor_widget):
            nonlocal count
            if not editor_widget:
                return
            editor_widget.setEffectNameOrder(_ALLOWED_EFFECTS)
            if hasattr(editor_widget, "setUnorderedEffectsVisible"):
                editor_widget.setUnorderedEffectsVisible(False)
            elif hasattr(editor_widget, "unorderedEffectsVisible"):
                editor_widget.unorderedEffectsVisible = False
            count += 1

        # 1) SegmentEditor 模块
        if hasattr(slicer.modules, "segmenteditor"):
            wr = slicer.modules.segmenteditor.widgetRepresentation()
            if wr:
                editor = slicer.util.findChild(wr, "qMRMLSegmentEditorWidget")
                apply_to_widget(editor)
                mw = wr.self() if hasattr(wr, "self") else None
                if mw and hasattr(mw, "editor"):
                    apply_to_widget(mw.editor)

        # 2) 主窗口内所有编辑器控件（含 Segmentations 内嵌）
        main_win = slicer.util.mainWindow()
        if main_win:
            for w in slicer.util.findChildren(main_win, "qMRMLSegmentEditorWidget"):
                apply_to_widget(w)

        if count > 0:
            _markerLogDebug("Segment Editor effects configured, count=%d" % count)
        if _VISIONMAGIC_DEBUG:
            print("VisionMagic: Segment Editor effects configured, count=%d" % count)
        return count > 0
    except Exception as e:
        _appendMarkerLog("configureSegmentEditorEffects error: %s" % str(e))
        print("VisionMagic: Failed to configure Segment Editor effects: %s" % str(e))
    return False


def _qt_property(obj, name, default=""):
    """PythonQt 下 text/objectName 等常为属性；标准 PyQt 则为方法。"""
    if obj is None:
        return default
    try:
        value = getattr(obj, name, default)
        if callable(value):
            return value() if value() is not None else default
        return value if value is not None else default
    except Exception:
        return default


def _qobject_name(obj):
    return str(_qt_property(obj, "objectName", ""))


def _qaction_text(action):
    return str(_qt_property(action, "text", ""))


def _qaction_menu(action):
    try:
        menu = getattr(action, "menu", None)
        if callable(menu):
            return menu()
        return menu
    except Exception:
        return None


def _qaction_is_separator(action):
    try:
        value = getattr(action, "isSeparator", False)
        if callable(value):
            return bool(value())
        return bool(value)
    except Exception:
        return False


def _hideActionByName(root, name):
    """按 objectName 隐藏 QAction。PythonQt 下 findChildren 需用字符串类名。"""
    if not root:
        return
    for a in root.findChildren("QAction"):
        if _qobject_name(a) == name:
            a.setVisible(False)
            a.setEnabled(False)
            return


def _hideActionsContainingText(mw, needles):
    """按菜单项文本关键词隐藏（菜单栏及 FileMenu 等顶层菜单）。"""
    if not mw:
        return
    menus = []
    try:
        menubar = mw.menuBar()
        if menubar:
            menus.extend(menubar.findChildren("QMenu"))
    except Exception:
        pass
    try:
        file_menu = slicer.util.lookupTopLevelWidget("FileMenu")
        if file_menu and file_menu not in menus:
            menus.append(file_menu)
    except Exception:
        pass
    for menu in menus:
        for a in menu.actions():
            t = _qaction_text(a)
            for needle in needles:
                if needle.lower() in t.lower():
                    a.setVisible(False)
                    a.setEnabled(False)
                    break


def _isDownloadSampleDataMenuAction(action):
    if not action or _qaction_is_separator(action):
        return False
    text = _qaction_text(action).replace("&", "").strip()
    if any(k in text for k in ("下载示例数据", "下载示例", "Download Sample Data")):
        return True
    tip = str(_qt_property(action, "toolTip", ""))
    if "SampleData" in tip or "sample data" in tip.lower():
        return True
    return False


def _removeDownloadSampleDataFromMenu(menu):
    if not menu:
        return False
    removed = False
    for action in list(menu.actions()):
        if _isDownloadSampleDataMenuAction(action):
            menu.removeAction(action)
            action.setVisible(False)
            action.setEnabled(False)
            removed = True
    return removed


def _patchSampleDataAddMenu():
    """阻止 SampleData 模块向文件菜单插入「下载示例数据」。"""
    module_class = None
    for mod_name in ("SampleData",):
        mod = sys.modules.get(mod_name)
        if mod and hasattr(mod, "SampleData"):
            module_class = mod.SampleData
            break
    if not module_class:
        try:
            import SampleData as sample_data_mod
            module_class = sample_data_mod.SampleData
        except ImportError:
            return False
    if getattr(module_class, "_radianceSkipDownloadMenu", False):
        return True
    original_add_menu = module_class.addMenu

    def patched_add_menu(self):
        return

    module_class.addMenu = patched_add_menu
    module_class._radianceSkipDownloadMenu = True
    _markerLogDebug("_patchSampleDataAddMenu: ok")
    return True


_sample_data_menu_poll_timer = None
_sample_data_menu_poll_count = 0
_SAMPLE_DATA_MENU_POLL_MAX = 150


def _stopSampleDataMenuHidePoll():
    global _sample_data_menu_poll_timer
    if _sample_data_menu_poll_timer:
        _sample_data_menu_poll_timer.stop()


def _sampleDataMenuHidePollTick():
    global _sample_data_menu_poll_count
    _sample_data_menu_poll_count += 1
    if hideDownloadSampleDataAction():
        _stopSampleDataMenuHidePoll()
        return
    if _sample_data_menu_poll_count >= _SAMPLE_DATA_MENU_POLL_MAX:
        _stopSampleDataMenuHidePoll()


def _startSampleDataMenuHidePoll():
    global _sample_data_menu_poll_timer, _sample_data_menu_poll_count
    _sample_data_menu_poll_count = 0
    if _sample_data_menu_poll_timer is None:
        _sample_data_menu_poll_timer = qt.QTimer()
        _sample_data_menu_poll_timer.setInterval(100)
        _sample_data_menu_poll_timer.connect("timeout()", _sampleDataMenuHidePollTick)
    if not _sample_data_menu_poll_timer.isActive():
        _sample_data_menu_poll_timer.start()
    _sampleDataMenuHidePollTick()


def _scheduleHideDownloadSampleData():
    _patchSampleDataAddMenu()
    for delay_ms in (0, 50, 100, 250, 500, 1000, 2000, 5000, 10000):
        qt.QTimer.singleShot(delay_ms, hideDownloadSampleDataAction)


def hideDownloadSampleDataAction(mw=None):
    """从文件菜单移除「下载示例数据」（插入晚于 startupCompleted，需补丁+removeAction）。"""
    mw = mw or slicer.util.mainWindow()
    if not mw:
        return False
    _patchSampleDataAddMenu()
    removed = False
    try:
        file_menu = slicer.util.lookupTopLevelWidget("FileMenu")
        if file_menu and _removeDownloadSampleDataFromMenu(file_menu):
            removed = True
        if hasattr(mw, "FileMenu") and mw.FileMenu and _removeDownloadSampleDataFromMenu(mw.FileMenu):
            removed = True
        for menu in mw.findChildren("QMenu"):
            if _qobject_name(menu) == "FileMenu":
                if _removeDownloadSampleDataFromMenu(menu):
                    removed = True
        menubar = mw.menuBar()
        if menubar:
            for top_action in menubar.actions():
                sub_menu = _qaction_menu(top_action)
                if sub_menu and _removeDownloadSampleDataFromMenu(sub_menu):
                    removed = True
        if not removed:
            _hideActionsContainingText(mw, [
                "Download Sample Data",
                "下载示例数据",
                "下载示例",
            ])
    except Exception as e:
        _appendMarkerLog("hideDownloadSampleDataAction error: %s" % str(e))
    if removed:
        _markerLogDebug("hideDownloadSampleDataAction: removed from menu")
    return removed


def _hideDockWidgetByName(mw, name):
    """隐藏指定名称的 QDockWidget。"""
    if not mw:
        return
    for dock in mw.findChildren("QDockWidget"):
        if _qobject_name(dock) == name:
            dock.hide()
            dock.setVisible(False)
            dock.setEnabled(False)
            dock.setAllowedAreas(qt.Qt.NoDockWidgetArea)
            dock.setFeatures(qt.QDockWidget.NoDockWidgetFeatures)
            return


def hideUnwantedUIElements():
    """隐藏不需要的 UI 元素，与 C++ applyShellTweaks 互补。
    VISIONMAGIC_DEBUG=1 时跳过隐藏 Error Log 和 Python Console，便于调试。
    """
    mainWindow = slicer.util.mainWindow()
    if not mainWindow:
        _appendMarkerLog("hideUnwantedUIElements: mainWindow is None")
        return
    try:
        for n in ("HelpReportBugOrFeatureRequestAction", "HelpSearchFeatureRequestsAction",
                  "HelpDocumentationAction", "HelpBrowseTutorialsAction",
                  "HelpAcknowledgmentsAction", "HelpAboutSlicerAppAction"):
            _hideActionByName(mainWindow, n)
    except Exception as e:
        _appendMarkerLog("hideByHelpNames error: %s" % str(e))
    try:
        _hideActionsContainingText(mainWindow, [
            "Documentation", "Tutorial", "Acknowledg", "Feedback",
            "Report Bug", "Feature Request", "Slicer", "Extensions Manager",
            "Download Sample Data", "下载示例数据",
        ])
        hideDownloadSampleDataAction(mainWindow)
    except Exception as e:
        _appendMarkerLog("hideByText(help) error: %s" % str(e))
    # VISIONMAGIC_DEBUG=1 时保留 Python Console 和 Error Log 可见
    _skip_python_error_log = _VISIONMAGIC_DEBUG
    if not _skip_python_error_log:
        try:
            for n in ("ViewPythonInteractorAction", "ViewPythonConsoleAction",
                      "WindowPythonInteractorAction", "WindowPythonConsoleAction",
                      "WindowErrorLogAction"):
                _hideActionByName(mainWindow, n)
            _hideActionsContainingText(mainWindow, ["Python Interactor", "Python Console", "Error Log"])
        except Exception as e:
            _appendMarkerLog("hidePython error: %s" % str(e))
        try:
            _hideDockWidgetByName(mainWindow, "PythonConsoleDockWidget")
            _hideDockWidgetByName(mainWindow, "ErrorLogDockWidget")
        except Exception as e:
            _appendMarkerLog("hideDocks error: %s" % str(e))
    try:
        _hideActionByName(mainWindow, "ViewExtensionsManagerAction")
        _hideActionByName(mainWindow, "ExtensionsManagerAction")
    except Exception as e:
        _appendMarkerLog("hideExtMgr error: %s" % str(e))
    try:
        toolBars = mainWindow.findChildren("QToolBar")
        for tb in toolBars:
            if _qobject_name(tb) == "DialogToolBar":
                acts = tb.actions()
                for a in acts:
                    a.setVisible(False)
                    a.setEnabled(False)
                tb.hide()
                break
    except Exception as e:
        _appendMarkerLog("hideDialogToolBar error: %s" % str(e))
    try:
        hideThreeDViewMoreToolButton(mainWindow)
        _scheduleHideThreeDViewMoreToolButton()
    except Exception as e:
        _appendMarkerLog("hideThreeDViewMoreToolButton error: %s" % str(e))
    _markerLogDebug("hideUnwantedUIElements: done (debug=%s)" % _VISIONMAGIC_DEBUG)


_dicom_ui_hide_poll_timer = None
_dicom_ui_hide_poll_count = 0
_DICOM_UI_POLL_MAX = 250
_dicom_ui_patches_applied = False


def _hideSlicerDicomBrowserAdvanced(browser):
    """隐藏 SlicerDICOMBrowser.setup() 里创建的「高级」勾选及高级表格区。"""
    if not browser:
        return False
    hidden = False
    btn = getattr(browser, "advancedViewButton", None)
    if btn:
        btn.hide()
        btn.setVisible(False)
        btn.setEnabled(False)
        hidden = True
    frame = getattr(browser, "loadableTableFrame", None)
    if frame:
        frame.hide()
        frame.setVisible(False)
    return hidden


def _patchSlicerDICOMBrowserSetup():
    """在 DICOM 浏览器 setup 末尾立刻隐藏「高级」，避免先绘制再消失。"""
    global _dicom_ui_patches_applied
    widget_class = None
    try:
        from DICOMLib.DICOMBrowser import SlicerDICOMBrowser
        widget_class = SlicerDICOMBrowser
    except ImportError:
        for mod_name in ("DICOMLib.DICOMBrowser", "DICOMBrowser"):
            mod = sys.modules.get(mod_name)
            if mod and hasattr(mod, "SlicerDICOMBrowser"):
                widget_class = mod.SlicerDICOMBrowser
                break
    if not widget_class or getattr(widget_class, "_radianceHideAdvancedPatched", False):
        return widget_class is not None
    original_setup = widget_class.setup

    def patched_setup(self, showPreview=False):
        original_setup(self, showPreview)
        _hideSlicerDicomBrowserAdvanced(self)

    widget_class.setup = patched_setup
    widget_class._radianceHideAdvancedPatched = True
    _dicom_ui_patches_applied = True
    _markerLogDebug("_patchSlicerDICOMBrowserSetup: ok")
    return True


def _patchDICOMWidgetSetup():
    """在 DICOM 模块面板 setup 末尾隐藏网络/数据库高级区。"""
    widget_class = None
    for mod_name in ("DICOM", "dicom"):
        mod = sys.modules.get(mod_name)
        if mod and hasattr(mod, "DICOMWidget"):
            widget_class = mod.DICOMWidget
            break
    if not widget_class or getattr(widget_class, "_radianceHidePanelsPatched", False):
        return widget_class is not None
    original_setup = widget_class.setup

    def patched_setup(self):
        original_setup(self)
        ui = getattr(self, "ui", None)
        if ui:
            for frame_name in ("networkingFrame", "browserSettingsFrame"):
                frame = getattr(ui, frame_name, None)
                if frame:
                    frame.setVisible(False)
                    frame.visible = False

    widget_class.setup = patched_setup
    widget_class._radianceHidePanelsPatched = True
    return True


def hideDICOMBrowserAdvancedCheckbox(root=None):
    """隐藏 DICOM 浏览器底部「高级」勾选项（AdvancedViewCheckBox）。"""
    root = root or slicer.util.mainWindow()
    if not root:
        return False
    found = False
    try:
        cb = root.findChild("QCheckBox", "AdvancedViewCheckBox")
        if cb:
            cb.hide()
            cb.setVisible(False)
            cb.setEnabled(False)
            found = True
        for browser in root.findChildren("QWidget"):
            if hasattr(browser, "advancedViewButton") and _hideSlicerDicomBrowserAdvanced(browser):
                found = True
        if not found:
            for checkbox in root.findChildren("QCheckBox"):
                text = _qaction_text(checkbox).strip()
                if text in ("Advanced", "高级", "高级选项"):
                    checkbox.hide()
                    checkbox.setVisible(False)
                    checkbox.setEnabled(False)
                    found = True
                    break
    except Exception as e:
        _appendMarkerLog("hideDICOMBrowserAdvancedCheckbox error: %s" % str(e))
    return found


def hideDICOMAdvancedPanels():
    """隐藏 DICOM 模块中的网络、数据库设置等高级面板（与截图区域一致）。"""
    panels_ok = False
    if hasattr(slicer.modules, "dicom"):
        try:
            wr = slicer.modules.dicom.widgetRepresentation()
            if wr:
                dicom_widget = wr.self() if hasattr(wr, "self") else wr
                ui = getattr(dicom_widget, "ui", None)
                if ui:
                    panels_ok = True
                    for frame_name in ("networkingFrame", "browserSettingsFrame"):
                        frame = getattr(ui, frame_name, None)
                        if not frame:
                            panels_ok = False
                        else:
                            frame.setVisible(False)
                            frame.visible = False
        except Exception as e:
            _appendMarkerLog("hideDICOMAdvancedPanels error: %s" % str(e))
    checkbox_ok = hideDICOMBrowserAdvancedCheckbox(slicer.util.mainWindow())
    if panels_ok and checkbox_ok:
        _markerLogDebug("hideDICOMAdvancedPanels: ok")
    return panels_ok and checkbox_ok


def _applyRadianceDicomUiHides():
    """左下角 DICOM「高级」+ 模块高级面板；返回是否均已处理。"""
    _patchSlicerDICOMBrowserSetup()
    _patchDICOMWidgetSetup()
    return hideDICOMAdvancedPanels()


def _stopDicomUiHidePoll():
    global _dicom_ui_hide_poll_timer
    if _dicom_ui_hide_poll_timer:
        _dicom_ui_hide_poll_timer.stop()


def _dicomUiHidePollTick():
    global _dicom_ui_hide_poll_count
    _dicom_ui_hide_poll_count += 1
    if _applyRadianceDicomUiHides():
        _stopDicomUiHidePoll()
        return
    if _dicom_ui_hide_poll_count >= _DICOM_UI_POLL_MAX:
        _stopDicomUiHidePoll()


_MODELS_MODULE_PANEL_NAMES = ("DisplayButton", "ClippingButton")


def _hideWidgetByObjectName(root, object_name):
    if not root:
        return False
    widget = None
    for cls in ("QToolButton", "QPushButton", "QWidget"):
        widget = root.findChild(cls, object_name)
        if widget:
            break
    if not widget:
        for cls in ("QToolButton", "QPushButton", "QWidget"):
            for child in root.findChildren(cls):
                if _qobject_name(child) == object_name:
                    widget = child
                    break
            if widget:
                break
    if not widget:
        return False
    widget.hide()
    widget.setVisible(False)
    widget.setMaximumHeight(0)
    return True


def hideThreeDViewMoreToolButton(root=None):
    """隐藏 3D 视图控制器「更多」按钮（深度剥离 / 立体观看 / FPS）。"""
    root = root or slicer.util.mainWindow()
    if not root:
        return False
    hidden_any = False
    try:
        for btn in root.findChildren("QToolButton"):
            if _qobject_name(btn) != "MoreToolButton":
                continue
            btn.hide()
            btn.setVisible(False)
            btn.setEnabled(False)
            try:
                btn.setMaximumSize(0, 0)
            except Exception:
                btn.setMaximumHeight(0)
            hidden_any = True
    except Exception as e:
        _appendMarkerLog("hideThreeDViewMoreToolButton error: %s" % str(e))
    if hidden_any:
        _markerLogDebug("hideThreeDViewMoreToolButton: ok")
    return hidden_any


def _scheduleHideThreeDViewMoreToolButton():
    hideThreeDViewMoreToolButton()
    for delay_ms in (16, 50, 100, 300, 500, 1000, 2000, 5000):
        qt.QTimer.singleShot(delay_ms, hideThreeDViewMoreToolButton)


# 保存对话框：隐藏「只选已修改数据」「只选场景与已修改数据」两个快捷按钮（Slicer 5.8）
_SAVE_DIALOG_SELECT_BUTTON_NAMES = ("SelectDataButton", "SelectSceneDataButton")
_SAVE_DIALOG_TITLE_KEYWORDS = (
    "Save Scene",
    "Unsaved Data",
    "保存场景",
    "未保存",
)


def _widgetClassName(widget):
    try:
        mo = widget.metaObject()
        if mo:
            return str(mo.className())
    except Exception:
        pass
    return type(widget).__name__


def _isSaveDataDialog(widget):
    if not widget:
        return False
    cls = _widgetClassName(widget)
    if cls == "qSlicerSaveDataDialog" or "SaveDataDialog" in cls:
        return True
    title = str(_qt_property(widget, "windowTitle", ""))
    return any(k in title for k in _SAVE_DIALOG_TITLE_KEYWORDS)


def _iterSaveDataDialogs():
    seen = set()
    widgets = []
    try:
        widgets.extend(slicer.app.topLevelWidgets())
    except Exception:
        pass
    try:
        mw = slicer.util.mainWindow()
        if mw:
            widgets.append(mw)
            for w in mw.findChildren("QDialog"):
                widgets.append(w)
    except Exception:
        pass
    for w in widgets:
        if not w or id(w) in seen:
            continue
        seen.add(id(w))
        if _isSaveDataDialog(w):
            yield w


def _hideSaveDialogSelectButtonsOn(dialog):
    """在指定保存对话框上隐藏两个批量勾选按钮。"""
    if not dialog:
        return False
    hidden_any = False
    for btn_name in _SAVE_DIALOG_SELECT_BUTTON_NAMES:
        if _hideWidgetByObjectName(dialog, btn_name):
            hidden_any = True
    # 按 tooltip 兜底（中/英 UI）
    _tip_needles = (
        "select modified data only",
        "select scene",
        "modified data only",
        "只选择修改过的数据",
        "只选择场景和修改",
    )
    for btn in dialog.findChildren("QToolButton"):
        tip = str(_qt_property(btn, "toolTip", "")).lower()
        if not tip:
            continue
        if any(n in tip for n in _tip_needles):
            btn.hide()
            btn.setVisible(False)
            btn.setMaximumHeight(0)
            hidden_any = True
    return hidden_any


# qSlicerSaveDataDialog 列索引（与 Slicer Base/QTGUI/qSlicerSaveDataDialog_p.h 一致）
_SAVE_DIALOG_COL_NODE_NAME = 4
_SAVE_DIALOG_COL_NODE_TYPE = 5
_SAVE_DIALOG_COL_NODE_STATUS = 6

# 场景行状态在 populateScene() 中硬编码英文；节点类型为 GetNodeTagName()（如 SceneView）
_SAVE_DIALOG_STATUS_ZH = {
    "Modified": "已修改",
    "Not Modified": "未修改",
}
_SAVE_DIALOG_NODE_TAG_ZH = {
    "SceneView": "场景视图",
    "Scene View": "场景视图",
    "Scene": "场景",
}


def _qt_table_item_text(item):
    if not item:
        return ""
    try:
        text = item.text()
        if callable(text):
            text = text()
        return str(text) if text is not None else ""
    except Exception:
        return ""


def _set_table_item_text_mapped(item, mapping):
    if not item:
        return False
    text = _qt_table_item_text(item).strip()
    zh = mapping.get(text)
    if zh and text != zh:
        item.setText(zh)
        return True
    return False


def _saveDialogNodeTagZh():
    """合并 MRML 类名映射，生成保存对话框节点类型列的中文标签。"""
    mapping = dict(_SAVE_DIALOG_NODE_TAG_ZH)
    try:
        for vtk_class, label in _NODE_TYPE_LABELS_ZH.items():
            if not vtk_class.startswith("vtkMRML") or not vtk_class.endswith("Node"):
                continue
            tag = vtk_class[6:-4]
            if tag and tag not in mapping:
                mapping[tag] = label
    except NameError:
        pass
    return mapping


def _localizeSaveDataDialogTable(dialog):
    """汉化保存对话框表格中的节点类型、节点名称与状态列。"""
    if not dialog:
        return False
    table = dialog.findChild("QTableWidget", "FileWidget")
    if not table:
        return False
    tag_zh = _saveDialogNodeTagZh()
    applied = False
    try:
        row_count = table.rowCount
        if callable(row_count):
            row_count = row_count()
        for row in range(int(row_count)):
            name_item = table.item(row, _SAVE_DIALOG_COL_NODE_NAME)
            type_item = table.item(row, _SAVE_DIALOG_COL_NODE_TYPE)
            status_item = table.item(row, _SAVE_DIALOG_COL_NODE_STATUS)
            if _set_table_item_text_mapped(name_item, tag_zh):
                applied = True
            if _set_table_item_text_mapped(type_item, tag_zh):
                applied = True
            if _set_table_item_text_mapped(status_item, _SAVE_DIALOG_STATUS_ZH):
                applied = True
    except Exception as e:
        _appendMarkerLog("_localizeSaveDataDialogTable error: %s" % str(e))
    return applied


# QDialogButtonBox 标准按钮（Qt 翻译未加载时常为英文）
_STANDARD_DIALOG_BUTTON_ZH = {
    "Reset": "重置",
    "OK": "确定",
    "Cancel": "取消",
    "Save": "保存",
}


def _stripButtonMnemonic(text):
    return str(text or "").replace("&", "").strip()


def _localizeDialogStandardButtons(dialog, extra_mapping=None):
    """汉化对话框底部 QDialogButtonBox 标准按钮。"""
    if not dialog:
        return False
    mapping = dict(_STANDARD_DIALOG_BUTTON_ZH)
    if extra_mapping:
        mapping.update(extra_mapping)
    applied = False
    try:
        for btn in dialog.findChildren("QAbstractButton"):
            try:
                raw = str(_qt_property(btn, "text", "")).strip()
                if not raw:
                    continue
                key = _stripButtonMnemonic(raw)
                zh = mapping.get(key)
                if zh and raw != zh:
                    btn.setText(zh)
                    applied = True
            except Exception:
                pass
    except Exception as e:
        _appendMarkerLog("_localizeDialogStandardButtons error: %s" % str(e))
    return applied


def _localizeSaveDataDialogButtons(dialog):
    """汉化保存数据对话框底部 Save / Cancel 等标准按钮。"""
    return _localizeDialogStandardButtons(dialog)


def localizeSaveDataDialogTables():
    """对所有已打开的保存对话框应用表格汉化。"""
    applied = False
    try:
        for dialog in _iterSaveDataDialogs():
            if _localizeSaveDataDialogTable(dialog):
                applied = True
    except Exception as e:
        _appendMarkerLog("localizeSaveDataDialogTables error: %s" % str(e))
    if applied:
        _markerLogDebug("localizeSaveDataDialogTables: ok")
    return applied


def hideSaveDataDialogSelectButtons():
    """隐藏 qSlicerSaveDataDialog 顶部的两个批量勾选按钮。"""
    hidden_any = False
    try:
        for dialog in _iterSaveDataDialogs():
            if _hideSaveDialogSelectButtonsOn(dialog):
                hidden_any = True
            if _localizeSaveDataDialogTable(dialog):
                hidden_any = True
            if _localizeSaveDataDialogButtons(dialog):
                hidden_any = True
    except Exception as e:
        _appendMarkerLog("hideSaveDataDialogSelectButtons error: %s" % str(e))
    if hidden_any:
        _markerLogDebug("hideSaveDataDialogSelectButtons: ok")
    return hidden_any


def _visibleSaveDataDialogExists():
    try:
        for dialog in _iterSaveDataDialogs():
            if dialog.isVisible():
                return True
    except Exception:
        pass
    return False


class _RadianceSaveDialogEventFilter(qt.QObject):
    """对话框 Show 时隐藏保存窗批量勾选按钮（PythonQt 无法覆写 ioManager 槽）。"""

    def eventFilter(self, watched, event):
        try:
            et = event.type()
            if et in (qt.QEvent.Show, qt.QEvent.ShowToParent, qt.QEvent.Polish):
                if _isSaveDataDialog(watched):
                    dialog = watched

                    def _apply():
                        _hideSaveDialogSelectButtonsOn(dialog)
                        _localizeSaveDataDialogTable(dialog)
                        _localizeSaveDataDialogButtons(dialog)
                        hideSaveDataDialogSelectButtons()

                    qt.QTimer.singleShot(0, _apply)
                    qt.QTimer.singleShot(32, _apply)
                    _startSaveDialogHidePoll()
        except Exception:
            pass
        return False


_save_dialog_event_filter = None


def _installSaveDialogEventFilter():
    global _save_dialog_event_filter
    if _save_dialog_event_filter is not None:
        return True
    try:
        _save_dialog_event_filter = _RadianceSaveDialogEventFilter()
        slicer.app.installEventFilter(_save_dialog_event_filter)
        _markerLogDebug("_installSaveDialogEventFilter: ok")
        return True
    except Exception as e:
        _appendMarkerLog("_installSaveDialogEventFilter error: %s" % str(e))
    return False


# QMessageBox 详情按钮（Show/Hide Details）：Slicer 退出保存等对话框会附带技术细节，产品 UI 不展示
_MSGBOX_DETAILS_BUTTON_LABELS = frozenset({
    "Show Details...",
    "Show Details",
    "Hide Details...",
    "Hide Details",
    "显示详细信息...",
    "显示详细信息",
    "隐藏详细信息...",
    "隐藏详细信息",
})


def _isMessageBox(widget):
    if not widget:
        return False
    try:
        cls = _widgetClassName(widget)
        if cls == "QMessageBox" or "MessageBox" in cls:
            return True
    except Exception:
        pass
    try:
        return isinstance(widget, qt.QMessageBox)
    except Exception:
        return False


def _isMessageBoxDetailsButton(btn):
    t = str(_qt_property(btn, "text", "")).strip()
    if t in _MSGBOX_DETAILS_BUTTON_LABELS:
        return True
    low = t.lower()
    return "details" in low and ("show" in low or "hide" in low or "详细" in t)


def _removeMessageBoxDetailsButton(msgbox):
    """移除 QMessageBox 的 Show/Hide Details 按钮并清空详情文本。"""
    if not msgbox:
        return False
    applied = False
    try:
        if hasattr(msgbox, "setDetailedText"):
            detailed = getattr(msgbox, "detailedText", None)
            if callable(detailed) and detailed():
                msgbox.setDetailedText("")
                applied = True
    except Exception:
        pass
    try:
        for btn in msgbox.findChildren("QAbstractButton"):
            try:
                if not _isMessageBoxDetailsButton(btn):
                    continue
                btn.hide()
                btn.setVisible(False)
                btn.setEnabled(False)
                btn.setMaximumSize(0, 0)
                applied = True
            except Exception:
                pass
    except Exception:
        return applied
    return applied


class _RadianceMessageBoxEventFilter(qt.QObject):
    """QMessageBox 弹出时移除 details 按钮。"""

    def eventFilter(self, watched, event):
        try:
            et = event.type()
            if et in (qt.QEvent.Show, qt.QEvent.ShowToParent, qt.QEvent.Polish):
                if _isMessageBox(watched):
                    box = watched

                    def _apply():
                        _removeMessageBoxDetailsButton(box)

                    qt.QTimer.singleShot(0, _apply)
                    qt.QTimer.singleShot(32, _apply)
        except Exception:
            pass
        return False


_msgbox_event_filter = None


def _installMessageBoxDetailsRemovalFilter():
    global _msgbox_event_filter
    if _msgbox_event_filter is not None:
        return True
    try:
        _msgbox_event_filter = _RadianceMessageBoxEventFilter()
        slicer.app.installEventFilter(_msgbox_event_filter)
        _markerLogDebug("_installMessageBoxDetailsRemovalFilter: ok")
        return True
    except Exception as e:
        _appendMarkerLog("_installMessageBoxDetailsRemovalFilter error: %s" % str(e))
    return False


# qSlicerDataDialog 底部 Reset/OK/Cancel 来自 QDialogButtonBox 标准按钮，Qt 翻译未加载时仍为英文
_ADD_DATA_DIALOG_TITLE_KEYWORDS = (
    "添加数据到场景中",
    "Add data into the scene",
)


def _isAddDataDialog(widget):
    if not widget:
        return False
    cls = _widgetClassName(widget)
    if cls == "qSlicerDataDialog" or "SlicerDataDialog" in cls:
        return True
    title = str(_qt_property(widget, "windowTitle", ""))
    return any(k in title for k in _ADD_DATA_DIALOG_TITLE_KEYWORDS)


def _localizeAddDataDialogButtons(dialog):
    """汉化「添加数据到场景中」对话框底部 Reset / OK / Cancel。"""
    return _localizeDialogStandardButtons(dialog)


class _RadianceAddDataDialogEventFilter(qt.QObject):
    """qSlicerDataDialog 弹出时汉化标准按钮。"""

    def eventFilter(self, watched, event):
        try:
            et = event.type()
            if et in (qt.QEvent.Show, qt.QEvent.ShowToParent, qt.QEvent.Polish):
                if _isAddDataDialog(watched):
                    dialog = watched

                    def _apply():
                        _localizeAddDataDialogButtons(dialog)

                    qt.QTimer.singleShot(0, _apply)
                    qt.QTimer.singleShot(32, _apply)
        except Exception:
            pass
        return False


_add_data_dialog_event_filter = None


def _installAddDataDialogI18nFilter():
    global _add_data_dialog_event_filter
    if _add_data_dialog_event_filter is not None:
        return True
    try:
        _add_data_dialog_event_filter = _RadianceAddDataDialogEventFilter()
        slicer.app.installEventFilter(_add_data_dialog_event_filter)
        _markerLogDebug("_installAddDataDialogI18nFilter: ok")
        return True
    except Exception as e:
        _appendMarkerLog("_installAddDataDialogI18nFilter error: %s" % str(e))
    return False


def _hookAddDataMenuActions(mw=None):
    """打开「添加数据」对话框前再兜底汉化一次。"""
    mw = mw or slicer.util.mainWindow()
    if not mw:
        return False
    hooked = False
    for action_name in ("FileAddDataAction", "LoadDataAction"):
        try:
            action = mw.findChild("QAction", action_name)
            if not action:
                continue
            if getattr(action, "_radianceAddDataHooked", False):
                hooked = True
                continue

            def _on_triggered():
                for delay_ms in (0, 50, 150):
                    qt.QTimer.singleShot(delay_ms, _localizeOpenAddDataDialogs)

            action.connect("triggered()", _on_triggered)
            action._radianceAddDataHooked = True
            hooked = True
            _markerLogDebug("_hookAddDataMenuActions: %s" % action_name)
        except Exception:
            pass
    return hooked


def _localizeOpenAddDataDialogs():
    applied = False
    try:
        for w in slicer.app.topLevelWidgets():
            if _isAddDataDialog(w) and w.isVisible():
                if _localizeAddDataDialogButtons(w):
                    applied = True
        mw = slicer.util.mainWindow()
        if mw:
            for w in mw.findChildren("QDialog"):
                if _isAddDataDialog(w) and w.isVisible():
                    if _localizeAddDataDialogButtons(w):
                        applied = True
    except Exception as e:
        _appendMarkerLog("_localizeOpenAddDataDialogs error: %s" % str(e))
    return applied


def _setupAddDataDialogI18n():
    _installAddDataDialogI18nFilter()
    _hookAddDataMenuActions()


def _hookSaveMenuActions(mw=None):
    """连接「保存」菜单/动作，在打开保存对话框前启动轮询。"""
    mw = mw or slicer.util.mainWindow()
    if not mw:
        return False
    hooked = False
    for action_name in ("FileSaveSceneAction", "FileSaveAction"):
        try:
            action = mw.findChild("QAction", action_name)
            if not action:
                continue
            if getattr(action, "_radianceSaveHooked", False):
                hooked = True
                continue
            action.connect("triggered()", _startSaveDialogHidePoll)
            action._radianceSaveHooked = True
            hooked = True
            _markerLogDebug("_hookSaveMenuActions: %s" % action_name)
        except Exception:
            pass
    return hooked


_save_dialog_poll_timer = None
_save_dialog_poll_idle_ticks = 0
_SAVE_DIALOG_POLL_IDLE_STOP = 400


def _saveDialogHidePollTick():
    global _save_dialog_poll_idle_ticks
    if _visibleSaveDataDialogExists():
        hideSaveDataDialogSelectButtons()
        localizeSaveDataDialogTables()
        _save_dialog_poll_idle_ticks = 0
        return
    if hideSaveDataDialogSelectButtons() or localizeSaveDataDialogTables():
        _save_dialog_poll_idle_ticks = 0
        return
    _save_dialog_poll_idle_ticks += 1
    if _save_dialog_poll_idle_ticks >= _SAVE_DIALOG_POLL_IDLE_STOP:
        _stopSaveDialogHidePoll()


def _stopSaveDialogHidePoll():
    global _save_dialog_poll_timer, _save_dialog_poll_idle_ticks
    _save_dialog_poll_idle_ticks = 0
    if _save_dialog_poll_timer:
        _save_dialog_poll_timer.stop()


def _startSaveDialogHidePoll():
    """保存对话框为模态弹出，轮询直到按钮隐藏且表格文案已汉化。"""
    global _save_dialog_poll_timer, _save_dialog_poll_idle_ticks
    _save_dialog_poll_idle_ticks = 0
    hideSaveDataDialogSelectButtons()
    localizeSaveDataDialogTables()
    if _save_dialog_poll_timer is None:
        _save_dialog_poll_timer = qt.QTimer()
        _save_dialog_poll_timer.setInterval(16)
        _save_dialog_poll_timer.connect("timeout()", _saveDialogHidePollTick)
    if not _save_dialog_poll_timer.isActive():
        _save_dialog_poll_timer.start()
    _saveDialogHidePollTick()


def _setupSaveDialogHiding():
    _installSaveDialogEventFilter()
    _hookSaveMenuActions()


# MRML 节点类型在部分环境下显示为英文类名缩写（如 LabelMapVolume、Markup），运行时覆盖为中文
_NODE_TYPE_LABELS_ZH = {
    "vtkMRMLLabelMapVolumeNode": "标签图",
    "vtkMRMLScalarVolumeNode": "标量体数据",
    "vtkMRMLVolumeNode": "体数据",
    "vtkMRMLMarkupsNode": "标记",
    "vtkMRMLMarkupsFiducialNode": "基准点",
    "vtkMRMLMarkupsLineNode": "线段",
    "vtkMRMLMarkupsAngleNode": "角度",
    "vtkMRMLMarkupsCurveNode": "曲线",
    "vtkMRMLMarkupsClosedCurveNode": "闭合曲线",
    "vtkMRMLMarkupsPlaneNode": "平面",
    "vtkMRMLMarkupsROINode": "ROI",
}


def _applyChineseNodeTypeLabels(root=None):
    """为 qMRMLNodeComboBox 设置中文节点类型名（Volumes / Markups 工具栏等）。"""
    targets = []
    if root:
        targets.append(root)
    else:
        try:
            mw = slicer.util.mainWindow()
            if mw:
                targets.append(mw)
        except Exception:
            pass
    applied = False
    for target in targets:
        try:
            for combo in target.findChildren("qMRMLNodeComboBox"):
                for node_type, label in _NODE_TYPE_LABELS_ZH.items():
                    try:
                        combo.setNodeTypeLabel(label, node_type)
                        applied = True
                    except Exception:
                        pass
        except Exception:
            pass
    return applied


# 模块快捷按钮 tooltip 使用内部英文名（如 Markups），替换为中文模块名
_MODULE_SHORTCUT_TOOLTIP_ZH = {
    "Markups": "打开标记模块",
    "Volumes": "打开体数据模块",
    "SegmentEditor": "打开分割模块",
    "DICOM": "打开 DICOM 模块",
    "Models": "打开模型模块",
}


def _localizeModuleShortcutTooltips(root=None):
    targets = []
    if root:
        targets.append(root)
    else:
        try:
            mw = slicer.util.mainWindow()
            if mw:
                targets.append(mw)
        except Exception:
            pass
    applied = False
    for target in targets:
        try:
            for btn in target.findChildren("QPushButton"):
                oname = _qobject_name(btn)
                tip = str(_qt_property(btn, "toolTip", ""))
                if "module shortcut" not in oname.lower() and "打开" not in tip and "Open the" not in tip:
                    continue
                for module_key, zh_tip in _MODULE_SHORTCUT_TOOLTIP_ZH.items():
                    if module_key in oname or module_key in tip:
                        if tip != zh_tip:
                            btn.setToolTip(zh_tip)
                            applied = True
                        break
        except Exception:
            pass
    return applied


_mrml_node_i18n_poll_timer = None
_mrml_node_i18n_poll_count = 0
_MRML_NODE_I18N_POLL_MAX = 200


def _stopMrmlNodeTypeI18nPoll():
    global _mrml_node_i18n_poll_timer
    if _mrml_node_i18n_poll_timer:
        _mrml_node_i18n_poll_timer.stop()


def _mrmlNodeTypeI18nPollTick():
    global _mrml_node_i18n_poll_count
    _mrml_node_i18n_poll_count += 1
    if _applyChineseNodeTypeLabels() and _localizeModuleShortcutTooltips():
        _stopMrmlNodeTypeI18nPoll()
        return
    if _mrml_node_i18n_poll_count >= _MRML_NODE_I18N_POLL_MAX:
        _stopMrmlNodeTypeI18nPoll()


def _startMrmlNodeTypeI18nPoll():
    global _mrml_node_i18n_poll_timer, _mrml_node_i18n_poll_count
    _mrml_node_i18n_poll_count = 0
    _applyChineseNodeTypeLabels()
    _localizeModuleShortcutTooltips()
    if _mrml_node_i18n_poll_timer is None:
        _mrml_node_i18n_poll_timer = qt.QTimer()
        _mrml_node_i18n_poll_timer.setInterval(16)
        _mrml_node_i18n_poll_timer.connect("timeout()", _mrmlNodeTypeI18nPollTick)
    if not _mrml_node_i18n_poll_timer.isActive():
        _mrml_node_i18n_poll_timer.start()
    _mrmlNodeTypeI18nPollTick()


def _refreshMrmlNodeTypeLocalization():
    _applyChineseNodeTypeLabels()
    _localizeModuleShortcutTooltips()


def _setupMrmlNodeTypeLocalization():
    _refreshMrmlNodeTypeLocalization()
    for delay_ms in (100, 500, 1500):
        qt.QTimer.singleShot(delay_ms, _refreshMrmlNodeTypeLocalization)


# 场景视图恢复菜单（qMRMLSceneViewMenu）中 Restore/Delete 等为硬编码英文
_SCENE_VIEW_MENU_TITLE_ZH = {
    "SceneView": "场景视图",
    "Scene View": "场景视图",
}
_SCENE_VIEW_MENU_ACTION_ZH = {
    "Restore": "还原",
    "Delete": "删除",
}


def _localizeSceneViewSubMenu(menu, menu_action=None):
    """汉化单个场景视图子菜单（objectName=sceneViewMenu）。"""
    if not menu:
        return False
    applied = False
    if menu_action is None:
        try:
            menu_action = menu.menuAction()
        except Exception:
            menu_action = None
    if menu_action:
        title = _qaction_text(menu_action).replace("&", "").strip()
        zh_title = _SCENE_VIEW_MENU_TITLE_ZH.get(title)
        if zh_title and _qaction_text(menu_action) != zh_title:
            menu_action.setText(zh_title)
            applied = True
    for action in menu.actions():
        if _qaction_is_separator(action):
            continue
        label = _qaction_text(action).replace("&", "").strip()
        zh_label = _SCENE_VIEW_MENU_ACTION_ZH.get(label)
        if zh_label and _qaction_text(action) != zh_label:
            action.setText(zh_label)
            applied = True
    return applied


def _localizeSceneViewMenuTree(root_menu):
    """汉化 qMRMLSceneViewMenu 下所有场景视图条目。"""
    if not root_menu:
        return False
    applied = False
    for action in root_menu.actions():
        sub_menu = _qaction_menu(action)
        if sub_menu and _qobject_name(sub_menu) == "sceneViewMenu":
            if _localizeSceneViewSubMenu(sub_menu, action):
                applied = True
    return applied


def _hookSceneViewRootMenu(root_menu):
    if not root_menu or getattr(root_menu, "_radianceSceneViewRootHooked", False):
        return False

    def _on_about_to_show():
        _localizeSceneViewMenuTree(root_menu)

    try:
        root_menu.aboutToShow.connect(_on_about_to_show)
        root_menu._radianceSceneViewRootHooked = True
        _markerLogDebug("_hookSceneViewRootMenu: ok")
        return True
    except Exception:
        return False


def _findAndLocalizeSceneViewMenus(root=None):
    """查找工具栏场景视图菜单并汉化（菜单项会随 MRML 场景动态重建）。"""
    targets = []
    if root:
        targets.append(root)
    else:
        try:
            mw = slicer.util.mainWindow()
            if mw:
                targets.append(mw)
        except Exception:
            pass
    applied = False
    for target in targets:
        try:
            for root_menu in target.findChildren("qMRMLSceneViewMenu"):
                if _hookSceneViewRootMenu(root_menu):
                    applied = True
                if _localizeSceneViewMenuTree(root_menu):
                    applied = True
            for menu in target.findChildren("QMenu"):
                if _qobject_name(menu) != "sceneViewMenu":
                    continue
                menu_action = None
                try:
                    menu_action = menu.menuAction()
                except Exception:
                    pass
                if _localizeSceneViewSubMenu(menu, menu_action):
                    applied = True
        except Exception:
            pass
    return applied


_scene_view_menu_poll_timer = None
_scene_view_menu_poll_count = 0
_SCENE_VIEW_MENU_POLL_MAX = 400


def _stopSceneViewMenuI18nPoll():
    global _scene_view_menu_poll_timer
    if _scene_view_menu_poll_timer:
        _scene_view_menu_poll_timer.stop()


def _sceneViewMenuI18nPollTick():
    global _scene_view_menu_poll_count
    _scene_view_menu_poll_count += 1
    if _findAndLocalizeSceneViewMenus():
        _stopSceneViewMenuI18nPoll()
        return
    if _scene_view_menu_poll_count >= _SCENE_VIEW_MENU_POLL_MAX:
        _stopSceneViewMenuI18nPoll()


def _startSceneViewMenuI18nPoll():
    global _scene_view_menu_poll_timer, _scene_view_menu_poll_count
    _scene_view_menu_poll_count = 0
    _findAndLocalizeSceneViewMenus()
    if _scene_view_menu_poll_timer is None:
        _scene_view_menu_poll_timer = qt.QTimer()
        _scene_view_menu_poll_timer.setInterval(16)
        _scene_view_menu_poll_timer.connect("timeout()", _sceneViewMenuI18nPollTick)
    if not _scene_view_menu_poll_timer.isActive():
        _scene_view_menu_poll_timer.start()
    _sceneViewMenuI18nPollTick()


def _setupSceneViewMenuLocalization():
    _findAndLocalizeSceneViewMenus()
    _startSceneViewMenuI18nPoll()
    for delay_ms in (100, 500, 1500, 3000):
        qt.QTimer.singleShot(delay_ms, _findAndLocalizeSceneViewMenus)


def hideModelsModuleDisplayClipping():
    """隐藏 Models 模块中的「显示」「裁剪」折叠面板（保留节点树与信息）。"""
    root = slicer.util.mainWindow()
    if not root:
        return False
    hidden_names = set()
    try:
        for name in _MODELS_MODULE_PANEL_NAMES:
            if _hideWidgetByObjectName(root, name):
                hidden_names.add(name)
        if hasattr(slicer.modules, "models"):
            wr = slicer.modules.models.widgetRepresentation()
            if wr:
                widget = wr.self() if hasattr(wr, "self") else wr
                for name in _MODELS_MODULE_PANEL_NAMES:
                    if _hideWidgetByObjectName(widget, name):
                        hidden_names.add(name)
    except Exception as e:
        _appendMarkerLog("hideModelsModuleDisplayClipping error: %s" % str(e))
    if hidden_names:
        _markerLogDebug("hideModelsModuleDisplayClipping: %s" % ",".join(sorted(hidden_names)))
    return len(hidden_names) == len(_MODELS_MODULE_PANEL_NAMES)


_segment_editor_poll_timer = None
_segment_editor_poll_count = 0
_SEGMENT_EDITOR_POLL_MAX = 400
_models_panel_poll_timer = None
_models_panel_poll_count = 0
_MODELS_PANEL_POLL_MAX = 400


def _stopSegmentEditorEffectsPoll():
    global _segment_editor_poll_timer
    if _segment_editor_poll_timer:
        _segment_editor_poll_timer.stop()


def _segmentEditorEffectsPollTick():
    global _segment_editor_poll_count
    _segment_editor_poll_count += 1
    if configureSegmentEditorEffects():
        _stopSegmentEditorEffectsPoll()
        return
    if _segment_editor_poll_count >= _SEGMENT_EDITOR_POLL_MAX:
        _stopSegmentEditorEffectsPoll()


def _startSegmentEditorEffectsPoll():
    """分割编辑器控件常在 enter() 时才创建，16ms 轮询直到效果列表精简生效。"""
    global _segment_editor_poll_timer, _segment_editor_poll_count
    _segment_editor_poll_count = 0
    configureSegmentEditorEffects()
    if _segment_editor_poll_timer is None:
        _segment_editor_poll_timer = qt.QTimer()
        _segment_editor_poll_timer.setInterval(16)
        _segment_editor_poll_timer.connect("timeout()", _segmentEditorEffectsPollTick)
    if not _segment_editor_poll_timer.isActive():
        _segment_editor_poll_timer.start()
    _segmentEditorEffectsPollTick()


def _stopModelsPanelHidePoll():
    global _models_panel_poll_timer
    if _models_panel_poll_timer:
        _models_panel_poll_timer.stop()


def _modelsPanelHidePollTick():
    global _models_panel_poll_count
    _models_panel_poll_count += 1
    if hideModelsModuleDisplayClipping():
        _stopModelsPanelHidePoll()
        return
    if _models_panel_poll_count >= _MODELS_PANEL_POLL_MAX:
        _stopModelsPanelHidePoll()


def _startModelsPanelHidePoll():
    """Models 的显示/裁剪面板在首次进入模块后才创建，16ms 轮询直到隐藏完成。"""
    global _models_panel_poll_timer, _models_panel_poll_count
    _models_panel_poll_count = 0
    hideModelsModuleDisplayClipping()
    if _models_panel_poll_timer is None:
        _models_panel_poll_timer = qt.QTimer()
        _models_panel_poll_timer.setInterval(16)
        _models_panel_poll_timer.connect("timeout()", _modelsPanelHidePollTick)
    if not _models_panel_poll_timer.isActive():
        _models_panel_poll_timer.start()
    _modelsPanelHidePollTick()


def _tryConnectModuleSelector():
    """尽早连接模块切换信号（不等到 _runAfterStartup）。"""
    try:
        mainWin = slicer.util.mainWindow()
        if not mainWin or not hasattr(mainWin, "moduleSelector"):
            return False
        bar = mainWin.moduleSelector()
        if not bar or not hasattr(bar, "moduleSelected"):
            return False
        if getattr(bar, "_radianceModuleHookConnected", False):
            return True
        bar.connect("moduleSelected(QString)", _onModuleSelected)
        bar._radianceModuleHookConnected = True
        _markerLogDebug("_tryConnectModuleSelector: ok")
        return True
    except Exception:
        return False


def _startDicomUiHidePoll():
    """16ms 轮询，控件一创建就隐藏（仅 DICOM 左下角/高级区，不影响文件菜单）。"""
    global _dicom_ui_hide_poll_timer, _dicom_ui_hide_poll_count
    _dicom_ui_hide_poll_count = 0
    if _dicom_ui_hide_poll_timer is None:
        _dicom_ui_hide_poll_timer = qt.QTimer()
        _dicom_ui_hide_poll_timer.setInterval(16)
        _dicom_ui_hide_poll_timer.connect("timeout()", _dicomUiHidePollTick)
    if not _dicom_ui_hide_poll_timer.isActive():
        _dicom_ui_hide_poll_timer.start()
    _dicomUiHidePollTick()


def _onModuleSelected(moduleName):
    """模块切换时的回调：切换到分割编辑器时重新应用效果配置。"""
    if moduleName and str(moduleName).strip().upper() == "DICOM":
        _startDicomUiHidePoll()
    if moduleName and str(moduleName).strip() == "SampleData":
        _scheduleHideDownloadSampleData()
        _startSampleDataMenuHidePoll()
    if moduleName and str(moduleName).strip() == "Models":
        _startModelsPanelHidePoll()
    if moduleName and str(moduleName).strip() in ("Volumes", "Markups"):
        _startMrmlNodeTypeI18nPoll()
    if _is_segment_editor_module(moduleName):
        _startSegmentEditorEffectsPoll()


def onModuleLoaded(moduleName):
    """模块加载完成后的回调。"""
    if moduleName and str(moduleName).strip().upper() == "DICOM":
        _startDicomUiHidePoll()
    if moduleName and str(moduleName).strip() == "SampleData":
        _patchSampleDataAddMenu()
        _scheduleHideDownloadSampleData()
        _startSampleDataMenuHidePoll()
    if moduleName and str(moduleName).strip() == "Models":
        _startModelsPanelHidePoll()
    if moduleName and str(moduleName).strip() in ("Volumes", "Markups"):
        _startMrmlNodeTypeI18nPoll()
    if _is_segment_editor_module(moduleName):
        _startSegmentEditorEffectsPoll()


def onStartupCompleted():
    """应用程序启动完成后的回调（主窗口已显示）。"""
    _startDicomUiHidePoll()
    _scheduleHideDownloadSampleData()
    _startSampleDataMenuHidePoll()
    _installMessageBoxDetailsRemovalFilter()
    _setupAddDataDialogI18n()
    _setupSaveDialogHiding()
    _tryConnectModuleSelector()
    qt.QTimer.singleShot(0, _runAfterStartup)


def _runAfterStartup():
    """startupCompleted 后延迟执行，确保 mainWindow 及子控件已就绪。"""
    try:
        _markerLogDebug("_runAfterStartup: calling hideUnwantedUIElements")
        hideUnwantedUIElements()
        _applyRadianceDicomUiHides()
        hideDownloadSampleDataAction()
        _scheduleHideDownloadSampleData()
        _setupSaveDialogHiding()
        _setupAddDataDialogI18n()
        _setupMrmlNodeTypeLocalization()
        _setupSceneViewMenuLocalization()
        # 文件菜单项由 SampleData.startupCompleted 插入，单独调度；DICOM 左下角由轮询+补丁处理
        qt.QTimer.singleShot(1500, hideUnwantedUIElements)
    except Exception as e:
        _appendMarkerLog("_runAfterStartup error: %s" % str(e))

    _tryConnectModuleSelector()
    for delay_ms in (0, 200, 500, 1500, 3000):
        qt.QTimer.singleShot(delay_ms, _setupSaveDialogHiding)
        qt.QTimer.singleShot(delay_ms, _setupAddDataDialogI18n)


# ==========================================================================
# 主入口：注册回调和启动配置
# ==========================================================================

if hasattr(slicer.app, "moduleManager"):
    moduleManager = slicer.app.moduleManager()
    if moduleManager:
        moduleManager.connect("moduleLoaded(QString)", onModuleLoaded)
        try:
            if hasattr(moduleManager, "moduleSelected"):
                moduleManager.connect("moduleSelected(QString)", _onModuleSelected)
        except Exception:
            pass

# 使用 startupCompleted 信号：主窗口显示后才执行
slicer.app.connect("startupCompleted()", onStartupCompleted)
# 尽早轮询 DICOM 左下角「高级」等（不等待 300ms/1500ms）
_startDicomUiHidePoll()
_patchSampleDataAddMenu()
_scheduleHideDownloadSampleData()
_startSampleDataMenuHidePoll()
_installMessageBoxDetailsRemovalFilter()
_setupAddDataDialogI18n()
_setupSaveDialogHiding()
_setupMrmlNodeTypeLocalization()
_setupSceneViewMenuLocalization()
_scheduleHideThreeDViewMoreToolButton()
qt.QTimer.singleShot(0, _tryConnectModuleSelector)
qt.QTimer.singleShot(500, _setupSaveDialogHiding)
qt.QTimer.singleShot(500, _setupAddDataDialogI18n)
qt.QTimer.singleShot(500, _tryConnectModuleSelector)
# 备用：若 startupCompleted 未触发（如 RadianceApp 启动流程不同），5 秒后强制执行
qt.QTimer.singleShot(5000, lambda: (_markerLogDebug("fallback 5s timer"), onStartupCompleted()))
_markerLogDebug("registered startupCompleted + dicom ui poll + 5s fallback")
if _VISIONMAGIC_DEBUG:
    print("VisionMagic: Startup configuration script registered")
