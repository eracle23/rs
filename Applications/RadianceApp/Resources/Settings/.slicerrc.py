# -*- coding: utf-8 -*-
"""
VisionMagic 启动配置脚本
在 Slicer 启动时自动执行，用于配置 UI 精简和功能限制
"""

import slicer
import qt

def configureSegmentEditorEffects():
    """
    配置 Segment Editor 只显示需要的效果工具
    根据需求截图，保留以下效果：
    - Threshold（阈值）
    - Paint（画笔）
    - Erase（橡皮擦）
    - Smoothing（平滑）
    - Islands（孤岛）
    - Logical operators（逻辑运算）
    """
    try:
        # 获取 Segment Editor 模块
        segmentEditorModule = slicer.modules.segmenteditor
        if not segmentEditorModule:
            return
        
        # 获取 widget
        segmentEditorWidget = slicer.modules.segmenteditor.widgetRepresentation()
        if not segmentEditorWidget:
            return
        
        # 查找 qMRMLSegmentEditorWidget
        editorWidget = slicer.util.findChild(segmentEditorWidget, "qMRMLSegmentEditorWidget")
        if not editorWidget:
            return
        
        # 设置允许的效果（按显示顺序）
        allowedEffects = [
            "Threshold",
            "Paint", 
            "Erase",
            "Smoothing",
            "Islands",
            "Logical operators"
        ]
        
        # 配置效果显示
        editorWidget.setEffectNameOrder(allowedEffects)
        editorWidget.setUnorderedEffectsVisible(False)
        
        print("VisionMagic: Segment Editor effects configured successfully")
        
    except Exception as e:
        print(f"VisionMagic: Failed to configure Segment Editor effects: {e}")


def hideUnwantedUIElements():
    """
    隐藏不需要的 UI 元素
    """
    try:
        mainWindow = slicer.util.mainWindow()
        if not mainWindow:
            return
        
        # 隐藏不需要的菜单项和工具栏按钮
        # 这些已经在 C++ 代码中处理，这里作为备份
        
        print("VisionMagic: UI elements configured successfully")
        
    except Exception as e:
        print(f"VisionMagic: Failed to configure UI elements: {e}")


def onModuleAboutToBeLoaded(moduleName):
    """
    模块即将加载时的回调
    """
    pass


def onModuleLoaded(moduleName):
    """
    模块加载完成后的回调
    用于在 Segment Editor 模块加载后配置效果
    """
    if moduleName == "SegmentEditor":
        # 延迟执行配置，确保 UI 完全初始化
        qt.QTimer.singleShot(500, configureSegmentEditorEffects)


def onStartupCompleted():
    """
    应用程序启动完成后的回调
    """
    print("VisionMagic: Application startup completed")
    hideUnwantedUIElements()
    
    # 如果 Segment Editor 已加载，配置效果
    if hasattr(slicer.modules, 'segmenteditor'):
        qt.QTimer.singleShot(1000, configureSegmentEditorEffects)


# ==========================================================================
# 主入口：注册回调和启动配置
# ==========================================================================

# 连接模块加载信号
if hasattr(slicer.app, 'moduleManager'):
    moduleManager = slicer.app.moduleManager()
    if moduleManager:
        moduleManager.connect('moduleLoaded(QString)', onModuleLoaded)

# 延迟执行启动配置（等待主窗口完全初始化）
qt.QTimer.singleShot(2000, onStartupCompleted)

print("VisionMagic: Startup configuration script loaded")

