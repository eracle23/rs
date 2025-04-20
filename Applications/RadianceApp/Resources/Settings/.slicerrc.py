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
5. 调试时设置环境变量 VISIONMAGIC_DEBUG=1 可保留 Error Log 可见，便于查看 print 输出
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
    """追加日志到 marker 文件，便于排查。"""
    try:
        os.makedirs(_marker_dir, exist_ok=True)
        with open(_marker_file, "a", encoding="utf-8") as f:
            f.write("[%s] %s\n" % (datetime.now().isoformat(), msg))
    except Exception:
        pass


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
_appendMarkerLog("import slicer, qt ok")

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
        # 先写入 QSettings
        settings = qt.QSettings()
        settings.setValue("SegmentEditor/EffectNameOrder", _ALLOWED_EFFECTS)
        settings.setValue("SegmentEditor/UnorderedEffectsVisible", False)

        count = 0
        # 1) 通过 SegmentEditor 模块获取（若已加载）
        if hasattr(slicer.modules, "segmenteditor"):
            wr = slicer.modules.segmenteditor.widgetRepresentation()
            if wr:
                mw = wr.self()
                if mw and hasattr(mw, "editor") and mw.editor:
                    mw.editor.setEffectNameOrder(_ALLOWED_EFFECTS)
                    mw.editor.unorderedEffectsVisible = False  # 使用属性，非 setter 方法
                    count += 1
        # 2) 从主窗口查找所有 qMRMLSegmentEditorWidget
        mw = slicer.util.mainWindow()
        if mw:
            for w in mw.findChildren(qt.QWidget):
                if hasattr(w, "setEffectNameOrder"):
                    try:
                        w.setEffectNameOrder(_ALLOWED_EFFECTS)
                        if hasattr(w, "unorderedEffectsVisible"):
                            w.unorderedEffectsVisible = False
                        count += 1
                    except Exception:
                        pass
        if count > 0:
            _appendMarkerLog("Segment Editor effects configured, count=%d" % count)
        print("VisionMagic: Segment Editor effects configured, count=%d" % count)
    except Exception as e:
        _appendMarkerLog("configureSegmentEditorEffects error: %s" % str(e))
        print("VisionMagic: Failed to configure Segment Editor effects: %s" % str(e))


def _hideActionByName(root, name):
    """按 objectName 隐藏 QAction。PythonQt 下 findChildren 需用字符串类名。"""
    if not root:
        return
    for a in root.findChildren("QAction"):
        if a.objectName() == name:
            a.setVisible(False)
            a.setEnabled(False)
            return


def _hideActionsContainingText(mw, needles):
    """按菜单项文本关键词隐藏。"""
    if not mw:
        return
    try:
        menubar = mw.menuBar()
    except Exception:
        return
    if not menubar:
        return
    for menu in menubar.findChildren("QMenu"):
        for a in menu.actions():
            t = a.text()
            for needle in needles:
                if needle.lower() in t.lower():
                    a.setVisible(False)
                    a.setEnabled(False)
                    break


def _hideDockWidgetByName(mw, name):
    """隐藏指定名称的 QDockWidget。"""
    if not mw:
        return
    for dock in mw.findChildren("QDockWidget"):
        if dock.objectName() == name:
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
            "Documentation", "Tutorial", "Acknowledg", "About", "Feedback",
            "Report Bug", "Feature Request", "Slicer", "Extensions Manager"
        ])
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
            if tb.objectName() == "DialogToolBar":
                acts = tb.actions()
                for a in acts:
                    a.setVisible(False)
                    a.setEnabled(False)
                tb.hide()
                break
    except Exception as e:
        _appendMarkerLog("hideDialogToolBar error: %s" % str(e))
    _appendMarkerLog("hideUnwantedUIElements: done (debug=%s)" % _VISIONMAGIC_DEBUG)


def _onModuleSelected(moduleName):
    """模块切换时的回调：切换到分割编辑器时重新应用效果配置。"""
    if _is_segment_editor_module(moduleName):
        qt.QTimer.singleShot(800, configureSegmentEditorEffects)


def onModuleLoaded(moduleName):
    """模块加载完成后的回调。"""
    if _is_segment_editor_module(moduleName):
        qt.QTimer.singleShot(1000, configureSegmentEditorEffects)


def onStartupCompleted():
    """应用程序启动完成后的回调（主窗口已显示）。"""
    # 延迟 300ms 确保所有子控件已创建
    qt.QTimer.singleShot(300, _runAfterStartup)


def _runAfterStartup():
    """startupCompleted 后延迟执行，确保 mainWindow 及子控件已就绪。"""
    try:
        _appendMarkerLog("_runAfterStartup: calling hideUnwantedUIElements")
        hideUnwantedUIElements()
        # 再次延迟执行一次，应对部分控件延迟创建
        qt.QTimer.singleShot(1500, hideUnwantedUIElements)
    except Exception as e:
        _appendMarkerLog("_runAfterStartup error: %s" % str(e))

    # 通过 moduleSelector 连接"切换模块"信号（用户从下拉框切换到分割编辑器时也生效）
    try:
        mainWin = slicer.util.mainWindow()
        if mainWin and hasattr(mainWin, "moduleSelector"):
            bar = mainWin.moduleSelector()
            if bar and hasattr(bar, "moduleSelected"):
                bar.connect("moduleSelected(QString)", _onModuleSelected)
    except Exception:
        pass

    if hasattr(slicer.modules, "segmenteditor"):
        qt.QTimer.singleShot(1000, configureSegmentEditorEffects)


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
# 备用：若 startupCompleted 未触发（如 RadianceApp 启动流程不同），5 秒后强制执行
qt.QTimer.singleShot(5000, lambda: (_appendMarkerLog("fallback 5s timer"), onStartupCompleted()))
_appendMarkerLog("registered startupCompleted + 5s fallback")
print("VisionMagic: Startup configuration script registered")
