/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#ifndef __RadianceShellCleaner_h
#define __RadianceShellCleaner_h

#include "qRadianceAppExport.h"

class qRadianceAppMainWindow;

/**
 * @brief Slicer 壳层净化工具
 *
 * 负责隐藏/禁用不属于 RadianceApp 的 Slicer 原生 UI 元素，包括：
 * - Help 菜单中的 Slicer 相关入口
 * - Python Console / Python Interactor / Error Log
 * - 模块搜索按钮
 * - 扩展管理器按钮与 DialogToolBar
 * - 布局菜单白名单过滤
 * - 首次显示时的初始化（DICOM 模块、模块下拉精简、窗口居中）
 *
 * 所有方法均为静态，无需实例化。
 */
class Q_RADIANCE_APP_EXPORT RadianceShellCleaner
{
public:
  /**
   * @brief 对主窗口执行全部壳层净化操作
   * @param window 目标主窗口，不得为 nullptr
   */
  static void apply(qRadianceAppMainWindow* window);
};

#endif // __RadianceShellCleaner_h
