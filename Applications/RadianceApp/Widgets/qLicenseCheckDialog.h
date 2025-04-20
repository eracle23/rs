/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#ifndef __qLicenseCheckDialog_h
#define __qLicenseCheckDialog_h

// Radiance includes
#include "qRadianceAppExport.h"

#include <QDialog>
#include <QScopedPointer>

class qLicenseCheckDialogPrivate;

/**
 * @brief 授权检查对话框
 * 
 * 在软件启动时显示，用于检测授权状态和显示版本信息。
 */
class Q_RADIANCE_APP_EXPORT qLicenseCheckDialog : public QDialog
{
  Q_OBJECT

public:
  explicit qLicenseCheckDialog(QWidget* parent = nullptr);
  virtual ~qLicenseCheckDialog();

  /**
   * @brief 执行登录与授权检查（先登录，通过后再验证授权）
   * @return QDialog::Accepted 登录且授权通过，QDialog::Rejected 用户取消或失败
   */
  int exec() override;

  /** 登录成功后取得的用户名（可供主程序使用） */
  QString loggedInUsername() const;

protected slots:
  /** 用户点击登录按钮：校验账号密码，通过后切换到授权验证页 */
  void onLoginClicked();

  /**
   * @brief 开始授权检查
   */
  void startLicenseCheck();

  /**
   * @brief 授权检查完成
   */
  void onLicenseCheckComplete(bool success);

  /**
   * @brief 更新进度条
   */
  void updateProgress();

protected:
  QScopedPointer<qLicenseCheckDialogPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qLicenseCheckDialog)
  Q_DISABLE_COPY(qLicenseCheckDialog)
};

#endif // __qLicenseCheckDialog_h

