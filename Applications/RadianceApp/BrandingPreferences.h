#ifndef __RadianceApp_BrandingPreferences_h
#define __RadianceApp_BrandingPreferences_h

#include <QtGlobal>
#include <QSettings>
#include <QString>
#include "qSlicerCoreApplication.h"

namespace RadianceBranding
{

/// 是否允许通过外观菜单切换深浅主题（默认 false：不可修改主题）
/// 优先从 DefaultSettings.ini 读取，其次从用户设置读取
inline bool themeSwitchAllowed()
{
  if (qSlicerCoreApplication* app = qSlicerCoreApplication::application())
    {
    if (QSettings* defaults = app->defaultSettings())
      {
      if (defaults->contains(QStringLiteral("Radiance/AllowThemeSwitch")))
        {
        return defaults->value(QStringLiteral("Radiance/AllowThemeSwitch")).toBool();
        }
      }
    }
  return QSettings().value(QStringLiteral("Radiance/AllowThemeSwitch"), false).toBool();
}

}

#endif
