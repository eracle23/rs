# -*- coding: utf-8 -*-
"""
VisionMagic UI 配置模块
此模块在应用启动时自动加载，用于配置 Segment Editor 效果和其他 UI 定制
"""

import os
import logging

import slicer
from slicer.ScriptedLoadableModule import *
import qt


class VisionMagicConfig(ScriptedLoadableModule):
    """
    VisionMagic 配置模块
    用于 UI 精简和功能配置
    """
    
    def __init__(self, parent):
        ScriptedLoadableModule.__init__(self, parent)
        self.parent.title = "VisionMagic Config"
        self.parent.categories = [""]  # 隐藏在模块列表中
        self.parent.dependencies = []
        self.parent.contributors = ["VisionMagic Team"]
        self.parent.helpText = ""
        self.parent.acknowledgementText = ""
        self.parent.hidden = True  # 隐藏此模块
        
        # 在模块加载后配置
        slicer.app.connect('startupCompleted()', self.onStartupCompleted)


    def onStartupCompleted(self):
        """应用启动完成后配置 UI"""
        # 延迟执行以确保所有模块都已加载
        qt.QTimer.singleShot(1000, self.configureUI)
    

    def configureUI(self):
        """配置 UI 精简"""
        self.configureSegmentEditorEffects()
        logging.info("VisionMagic: UI configuration completed")


    def configureSegmentEditorEffects(self):
        """
        配置 Segment Editor 只显示需要的效果工具
        """
        try:
            # 保留的效果列表
            allowedEffects = [
                "Threshold",      # 阈值
                "Paint",          # 画笔
                "Erase",          # 橡皮擦
                "Smoothing",      # 平滑
                "Islands",        # 孤岛
                "Logical operators"  # 逻辑运算
            ]
            
            # 设置默认效果顺序
            settings = qt.QSettings()
            settings.setValue("SegmentEditor/EffectNameOrder", allowedEffects)
            settings.setValue("SegmentEditor/UnorderedEffectsVisible", False)
            
            # 如果 Segment Editor 已打开，立即应用配置
            if hasattr(slicer.modules, 'segmenteditor'):
                widget = slicer.modules.segmenteditor.widgetRepresentation()
                if widget:
                    editorWidget = slicer.util.findChild(widget, "qMRMLSegmentEditorWidget")
                    if editorWidget:
                        editorWidget.setEffectNameOrder(allowedEffects)
                        editorWidget.setUnorderedEffectsVisible(False)
            
            logging.info("VisionMagic: Segment Editor effects configured")
            
        except Exception as e:
            logging.warning(f"VisionMagic: Could not configure Segment Editor effects: {e}")


class VisionMagicConfigWidget(ScriptedLoadableModuleWidget):
    """此模块没有可见的 UI"""
    
    def setup(self):
        ScriptedLoadableModuleWidget.setup(self)
        # 此模块没有可见的 UI
        self.layout.addStretch(1)

