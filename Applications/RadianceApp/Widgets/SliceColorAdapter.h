/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#ifndef __SliceColorAdapter_h
#define __SliceColorAdapter_h

#include "qRadianceAppExport.h"

#include <QColor>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>

class ThemeSync;

/**
 * @brief 切片节点颜色适配器
 *
 * 订阅 ThemeSync::brandingApplied 信号，在主题更新后将
 * vtkMRMLSliceNode 的布局颜色调整到适合品牌主题的范围。
 *
 * 将 MRML 操作从 ThemeSync 中分离，使 ThemeSync 只依赖 Qt，
 * 本类单独承担跨层的 MRML 颜色同步职责。
 */
class Q_RADIANCE_APP_EXPORT SliceColorAdapter : public QObject
{
  Q_OBJECT

public:
  /**
   * @param themeSync 要订阅的 ThemeSync 实例，不得为 nullptr
   * @param parent    Qt 父对象
   */
  explicit SliceColorAdapter(ThemeSync* themeSync, QObject* parent = nullptr);
  ~SliceColorAdapter() override = default;

public Q_SLOTS:
  /// 在品牌样式应用后同步所有切片节点的布局颜色
  void onBrandingApplied();

private:
  /// 已主动调整过颜色的节点（nodeId -> 调整后颜色）
  QHash<QString, QColor> tunedSliceNodeColors_;
  /// 用户手动修改过颜色的节点，不再自动调整
  QSet<QString> userOverriddenSliceNodes_;
};

#endif // __SliceColorAdapter_h
