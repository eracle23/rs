/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#include "SystemColorScheme.h"

#include <QApplication>
#include <QPalette>
#include <QSettings>

namespace SystemColorScheme
{

bool isDarkMode()
{
#ifdef Q_OS_WIN
  QSettings settings(
    QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
    QSettings::NativeFormat);
  // 1 = 浅色应用模式，0 = 深色应用模式
  return settings.value(QStringLiteral("AppsUseLightTheme"), 1).toInt() == 0;
#else
  if (QApplication::instance())
    {
    const QColor window = QApplication::palette().color(QPalette::Window);
    if (window.isValid())
      {
      return window.lightness() < 128;
      }
    }
  return true;
#endif
}

} // namespace SystemColorScheme
