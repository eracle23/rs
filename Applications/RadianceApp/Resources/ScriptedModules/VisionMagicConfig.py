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
import ctk


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
        
        # 尽早配置 DICOM 数据库路径（在 DICOM 模块加载前）
        self.configureDICOMDatabasePath()
        
        # 在模块加载后配置
        slicer.app.connect('startupCompleted()', self.onStartupCompleted)
    
    
    def configureDICOMDatabasePath(self):
        """
        配置 DICOM 数据库路径到程序目录下，避免用户目录中文路径问题
        """
        try:
            settings = qt.QSettings()
            schemaVersion = ctk.ctkDICOMDatabase().schemaVersion()
            settingsKey = f"DatabaseDirectory_{schemaVersion}"
            
            # 检查是否已经设置了路径
            existingPath = settings.value(settingsKey)
            if existingPath:
                # 已有设置，检查路径是否有效（无乱码）
                try:
                    existingPath.encode('ascii')
                    # 路径只包含 ASCII 字符，可能是安全的
                    if os.path.exists(existingPath) or not self._hasNonAsciiInPath(existingPath):
                        return  # 使用现有设置
                except UnicodeEncodeError:
                    pass  # 路径包含非 ASCII 字符，需要重新配置
            
            # 使用程序安装目录下的 DICOMDatabase 文件夹
            appDir = slicer.app.slicerHome
            dicomDbPath = os.path.join(appDir, "DICOMDatabase")
            
            # 确保路径存在
            if not os.path.exists(dicomDbPath):
                os.makedirs(dicomDbPath, exist_ok=True)
            
            # 保存到设置
            settings.setValue(settingsKey, dicomDbPath)
            logging.info(f"VisionMagic: DICOM database path configured to: {dicomDbPath}")
            
        except Exception as e:
            logging.warning(f"VisionMagic: Could not configure DICOM database path: {e}")
    
    
    def _hasNonAsciiInPath(self, path):
        """检查路径是否包含非 ASCII 字符"""
        try:
            path.encode('ascii')
            return False
        except UnicodeEncodeError:
            return True


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
                "Scissors",       # 剪刀
                "Margin",         # 边距
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

