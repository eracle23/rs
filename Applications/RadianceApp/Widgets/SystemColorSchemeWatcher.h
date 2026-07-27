/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#ifndef __SystemColorSchemeWatcher_h
#define __SystemColorSchemeWatcher_h

#include "qRadianceAppExport.h"

#include <QObject>

/// 监听操作系统深浅色模式变化（Windows WM_SETTINGCHANGE）
class Q_RADIANCE_APP_EXPORT SystemColorSchemeWatcher : public QObject
{
  Q_OBJECT

public:
  explicit SystemColorSchemeWatcher(QObject* parent = nullptr);
  ~SystemColorSchemeWatcher() override;

Q_SIGNALS:
  void schemeChanged();

private Q_SLOTS:
  void notifySchemeChanged();

private:
  class NativeFilter;
  NativeFilter* Filter{nullptr};
};

#endif
