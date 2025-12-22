/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#ifndef __LicenseManager_h
#define __LicenseManager_h

// Radiance includes
#include "qRadianceAppExport.h"

#include <QString>
#include <QObject>

/**
 * @brief 授权管理器 - 加密狗接口预留类
 * 
 * 此类提供授权验证的接口框架，当前实现为开发模式（始终通过）。
 * 后续集成具体加密狗 SDK 时，只需修改此类的实现即可。
 */
class Q_RADIANCE_APP_EXPORT LicenseManager : public QObject
{
  Q_OBJECT

public:
  /**
   * @brief 获取单例实例
   */
  static LicenseManager& instance();

  /**
   * @brief 检查授权是否有效
   * @return true 授权有效，false 授权无效
   * 
   * TODO: 集成加密狗 SDK 后实现实际检测逻辑
   * 当前实现：开发模式下始终返回 true
   */
  virtual bool checkLicense();

  /**
   * @brief 获取授权信息字符串
   * @return 授权信息描述
   */
  virtual QString getLicenseInfo();

  /**
   * @brief 是否为开发模式
   * @return true 开发模式（绕过授权检查），false 正式模式
   */
  virtual bool isDevelopmentMode();

  /**
   * @brief 设置开发模式
   * @param enabled 是否启用开发模式
   */
  void setDevelopmentMode(bool enabled);

  /**
   * @brief 获取授权状态描述
   */
  QString getStatusMessage();

  /**
   * @brief 获取版本信息
   */
  static QString getVersionString();
  static QString getReleaseVersion();

signals:
  /**
   * @brief 授权状态改变信号
   */
  void licenseStatusChanged(bool valid);

protected:
  LicenseManager();
  virtual ~LicenseManager();

private:
  Q_DISABLE_COPY(LicenseManager)
  
  bool m_developmentMode;
  bool m_licenseValid;
  QString m_licenseInfo;
};

#endif // __LicenseManager_h

