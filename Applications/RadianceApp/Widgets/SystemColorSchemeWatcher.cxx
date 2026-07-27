/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#include "SystemColorSchemeWatcher.h"

#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QMetaObject>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace
{

#ifdef Q_OS_WIN
class WindowsThemeNativeFilter : public QAbstractNativeEventFilter
{
public:
  explicit WindowsThemeNativeFilter(SystemColorSchemeWatcher* owner)
    : Owner(owner)
  {
  }

  bool nativeEventFilter(const QByteArray& eventType, void* message, long*) override
  {
    if (!this->Owner)
      {
      return false;
      }
    if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG")
      {
      return false;
      }

    const MSG* msg = static_cast<const MSG*>(message);
    if (msg->message != WM_SETTINGCHANGE || msg->lParam == 0)
      {
      return false;
      }

    const wchar_t* section = reinterpret_cast<const wchar_t*>(msg->lParam);
    if (wcscmp(section, L"ImmersiveColorSet") != 0 &&
        wcscmp(section, L"WindowsThemeElement") != 0)
      {
      return false;
      }

    // 注册表可能尚未更新，排队到事件循环再读取（invokeMethod 可调用 private slot）
    QMetaObject::invokeMethod(this->Owner, "notifySchemeChanged", Qt::QueuedConnection);
    return false;
  }

  SystemColorSchemeWatcher* Owner;
};
#endif

} // namespace

class SystemColorSchemeWatcher::NativeFilter
#ifdef Q_OS_WIN
  : public WindowsThemeNativeFilter
#endif
{
public:
#ifdef Q_OS_WIN
  explicit NativeFilter(SystemColorSchemeWatcher* owner)
    : WindowsThemeNativeFilter(owner)
  {
  }
#else
  explicit NativeFilter(SystemColorSchemeWatcher*)
  {
  }
#endif
};

SystemColorSchemeWatcher::SystemColorSchemeWatcher(QObject* parent)
  : QObject(parent)
{
#ifdef Q_OS_WIN
  this->Filter = new NativeFilter(this);
  if (qApp)
    {
    qApp->installNativeEventFilter(this->Filter);
    }
#endif
}

SystemColorSchemeWatcher::~SystemColorSchemeWatcher()
{
#ifdef Q_OS_WIN
  if (this->Filter && qApp)
    {
    qApp->removeNativeEventFilter(this->Filter);
    }
  delete this->Filter;
  this->Filter = nullptr;
#endif
}

void SystemColorSchemeWatcher::notifySchemeChanged()
{
  emit this->schemeChanged();
}
