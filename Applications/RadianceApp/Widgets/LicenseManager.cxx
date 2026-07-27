/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#include "LicenseManager.h"
#include "RadianceAppVersion.h" // ${CMAKE_CURRENT_BINARY_DIR}/RadianceAppVersion.h

// Qt includes
#include <QCoreApplication>

//-----------------------------------------------------------------------------
LicenseManager& LicenseManager::instance()
{
  static LicenseManager instance;
  return instance;
}

//-----------------------------------------------------------------------------
LicenseManager::LicenseManager()
  : QObject(nullptr)
  , m_developmentMode(true)  // 默认开发模式，后续发布时改为 false
  , m_licenseValid(false)
  , m_licenseInfo()
{
}

//-----------------------------------------------------------------------------
LicenseManager::~LicenseManager()
{
}

//-----------------------------------------------------------------------------
bool LicenseManager::checkLicense()
{
  // 开发模式下始终通过
  if (m_developmentMode)
  {
    m_licenseValid = true;
    m_licenseInfo = QString::fromUtf8("开发模式 - 授权检查已跳过");
    emit licenseStatusChanged(true);
    return true;
  }

  // TODO: 在此处集成加密狗 SDK
  // 示例代码结构：
  // 
  // #ifdef USE_SENSELOCK
  //   // 深思洛克加密狗检测
  //   if (SenseLock_Find() == 0)
  //   {
  //     m_licenseValid = true;
  //     m_licenseInfo = "授权有效";
  //   }
  // #elif defined(USE_HASP)
  //   // SafeNet HASP 加密狗检测
  //   hasp_status_t status = hasp_login(...);
  //   if (status == HASP_STATUS_OK)
  //   {
  //     m_licenseValid = true;
  //     m_licenseInfo = "授权有效";
  //   }
  // #endif

  // 当前未集成加密狗，返回失败
  m_licenseValid = false;
  m_licenseInfo = QString::fromUtf8("未检测到有效的授权设备");
  emit licenseStatusChanged(false);
  return false;
}

//-----------------------------------------------------------------------------
QString LicenseManager::getLicenseInfo()
{
  return m_licenseInfo;
}

//-----------------------------------------------------------------------------
bool LicenseManager::isDevelopmentMode()
{
  return m_developmentMode;
}

//-----------------------------------------------------------------------------
void LicenseManager::setDevelopmentMode(bool enabled)
{
  m_developmentMode = enabled;
}

//-----------------------------------------------------------------------------
QString LicenseManager::getStatusMessage()
{
  if (m_developmentMode)
  {
    return QString::fromUtf8("开发模式");
  }
  else if (m_licenseValid)
  {
    return QString::fromUtf8("授权有效");
  }
  else
  {
    return QString::fromUtf8("未授权");
  }
}

//-----------------------------------------------------------------------------
QString LicenseManager::getVersionString()
{
  return RadianceAppVersion::displayVersionString();
}

//-----------------------------------------------------------------------------
QString LicenseManager::getReleaseVersion()
{
  return RadianceAppVersion::releaseVersionString();
}

