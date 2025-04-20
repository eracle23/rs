/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#include "qLicenseCheckDialog.h"
#include "LicenseManager.h"

// Qt includes
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QFont>
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QLinearGradient>

//-----------------------------------------------------------------------------
class qLicenseCheckDialogPrivate
{
  Q_DECLARE_PUBLIC(qLicenseCheckDialog)
protected:
  qLicenseCheckDialog* const q_ptr;

public:
  qLicenseCheckDialogPrivate(qLicenseCheckDialog& object);
  void setupUi();

  QLabel* TitleLabel;
  QLabel* SubtitleLabel;
  QLabel* VersionLabel;
  QLabel* ReleaseLabel;
  QLabel* StatusLabel;
  QProgressBar* ProgressBar;
  QPushButton* ExitButton;
  QTimer* ProgressTimer;
  int ProgressValue;
  bool CheckComplete;
  bool CheckSuccess;
};

//-----------------------------------------------------------------------------
qLicenseCheckDialogPrivate::qLicenseCheckDialogPrivate(qLicenseCheckDialog& object)
  : q_ptr(&object)
  , TitleLabel(nullptr)
  , SubtitleLabel(nullptr)
  , VersionLabel(nullptr)
  , ReleaseLabel(nullptr)
  , StatusLabel(nullptr)
  , ProgressBar(nullptr)
  , ExitButton(nullptr)
  , ProgressTimer(nullptr)
  , ProgressValue(0)
  , CheckComplete(false)
  , CheckSuccess(false)
{
}

//-----------------------------------------------------------------------------
void qLicenseCheckDialogPrivate::setupUi()
{
  Q_Q(qLicenseCheckDialog);

  q->setWindowTitle(QString::fromUtf8("授权验证"));
  q->setFixedSize(480, 320);
  q->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
  q->setAttribute(Qt::WA_TranslucentBackground, false);

  // 设置深色背景样式
  q->setStyleSheet(
    "qLicenseCheckDialog {"
    "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
    "    stop:0 #2c3e50, stop:1 #1a252f);"
    "  border: 2px solid #3498db;"
    "  border-radius: 10px;"
    "}"
  );

  QVBoxLayout* mainLayout = new QVBoxLayout(q);
  mainLayout->setContentsMargins(40, 30, 40, 30);
  mainLayout->setSpacing(12);

  // 标题
  TitleLabel = new QLabel(QString::fromUtf8("医学影像三维重建软件"));
  TitleLabel->setStyleSheet(
    "QLabel {"
    "  color: #ffffff;"
    "  font-size: 22px;"
    "  font-weight: bold;"
    "  background: transparent;"
    "}"
  );
  TitleLabel->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(TitleLabel);

  // 副标题
  SubtitleLabel = new QLabel("Vision Magic Ecosystem");
  SubtitleLabel->setStyleSheet(
    "QLabel {"
    "  color: #3498db;"
    "  font-size: 14px;"
    "  background: transparent;"
    "}"
  );
  SubtitleLabel->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(SubtitleLabel);

  mainLayout->addSpacing(20);

  // 版本信息布局
  QHBoxLayout* versionLayout = new QHBoxLayout();
  versionLayout->setSpacing(40);

  // 定期版本号
  QVBoxLayout* versionCol1 = new QVBoxLayout();
  QLabel* versionTitleLabel = new QLabel(QString::fromUtf8("定期版本号"));
  versionTitleLabel->setStyleSheet("QLabel { color: #95a5a6; font-size: 12px; background: transparent; }");
  versionTitleLabel->setAlignment(Qt::AlignCenter);
  VersionLabel = new QLabel(LicenseManager::getVersionString());
  VersionLabel->setStyleSheet("QLabel { color: #ffffff; font-size: 16px; font-weight: bold; background: transparent; }");
  VersionLabel->setAlignment(Qt::AlignCenter);
  versionCol1->addWidget(versionTitleLabel);
  versionCol1->addWidget(VersionLabel);

  // 发布版本号
  QVBoxLayout* versionCol2 = new QVBoxLayout();
  QLabel* releaseTitleLabel = new QLabel(QString::fromUtf8("发布版本号"));
  releaseTitleLabel->setStyleSheet("QLabel { color: #95a5a6; font-size: 12px; background: transparent; }");
  releaseTitleLabel->setAlignment(Qt::AlignCenter);
  ReleaseLabel = new QLabel(LicenseManager::getReleaseVersion());
  ReleaseLabel->setStyleSheet("QLabel { color: #ffffff; font-size: 16px; font-weight: bold; background: transparent; }");
  ReleaseLabel->setAlignment(Qt::AlignCenter);
  versionCol2->addWidget(releaseTitleLabel);
  versionCol2->addWidget(ReleaseLabel);

  versionLayout->addStretch();
  versionLayout->addLayout(versionCol1);
  versionLayout->addLayout(versionCol2);
  versionLayout->addStretch();
  mainLayout->addLayout(versionLayout);

  mainLayout->addSpacing(20);

  // 进度条
  ProgressBar = new QProgressBar();
  ProgressBar->setRange(0, 100);
  ProgressBar->setValue(0);
  ProgressBar->setTextVisible(false);
  ProgressBar->setFixedHeight(8);
  ProgressBar->setStyleSheet(
    "QProgressBar {"
    "  background: #34495e;"
    "  border: none;"
    "  border-radius: 4px;"
    "}"
    "QProgressBar::chunk {"
    "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
    "    stop:0 #3498db, stop:1 #2ecc71);"
    "  border-radius: 4px;"
    "}"
  );
  mainLayout->addWidget(ProgressBar);

  // 状态标签
  StatusLabel = new QLabel(QString::fromUtf8("正在验证授权..."));
  StatusLabel->setStyleSheet(
    "QLabel {"
    "  color: #bdc3c7;"
    "  font-size: 13px;"
    "  background: transparent;"
    "}"
  );
  StatusLabel->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(StatusLabel);

  mainLayout->addStretch();

  // 退出按钮
  QHBoxLayout* buttonLayout = new QHBoxLayout();
  ExitButton = new QPushButton(QString::fromUtf8("退出"));
  ExitButton->setFixedSize(100, 32);
  ExitButton->setStyleSheet(
    "QPushButton {"
    "  background: #e74c3c;"
    "  color: white;"
    "  border: none;"
    "  border-radius: 4px;"
    "  font-size: 13px;"
    "}"
    "QPushButton:hover {"
    "  background: #c0392b;"
    "}"
    "QPushButton:pressed {"
    "  background: #a93226;"
    "}"
  );
  QObject::connect(ExitButton, &QPushButton::clicked, q, &QDialog::reject);
  buttonLayout->addStretch();
  buttonLayout->addWidget(ExitButton);
  buttonLayout->addStretch();
  mainLayout->addLayout(buttonLayout);

  // 进度定时器
  ProgressTimer = new QTimer(q);
  QObject::connect(ProgressTimer, &QTimer::timeout, q, &qLicenseCheckDialog::updateProgress);
}

//-----------------------------------------------------------------------------
qLicenseCheckDialog::qLicenseCheckDialog(QWidget* parent)
  : QDialog(parent)
  , d_ptr(new qLicenseCheckDialogPrivate(*this))
{
  Q_D(qLicenseCheckDialog);
  d->setupUi();

  // 居中显示
  if (QScreen* screen = QApplication::primaryScreen())
  {
    QRect screenGeometry = screen->availableGeometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
  }
}

//-----------------------------------------------------------------------------
qLicenseCheckDialog::~qLicenseCheckDialog()
{
}

//-----------------------------------------------------------------------------
int qLicenseCheckDialog::exec()
{
  Q_D(qLicenseCheckDialog);

  // 显示对话框
  show();
  QApplication::processEvents();

  // 延迟启动检查，让 UI 先渲染
  QTimer::singleShot(100, this, &qLicenseCheckDialog::startLicenseCheck);

  return QDialog::exec();
}

//-----------------------------------------------------------------------------
void qLicenseCheckDialog::startLicenseCheck()
{
  Q_D(qLicenseCheckDialog);

  d->ProgressValue = 0;
  d->CheckComplete = false;
  d->ProgressTimer->start(30);  // 30ms 更新一次进度

  // 在后台线程中执行授权检查（模拟）
  QTimer::singleShot(1500, this, [this]() {
    bool success = LicenseManager::instance().checkLicense();
    onLicenseCheckComplete(success);
  });
}

//-----------------------------------------------------------------------------
void qLicenseCheckDialog::updateProgress()
{
  Q_D(qLicenseCheckDialog);

  if (!d->CheckComplete && d->ProgressValue < 90)
  {
    d->ProgressValue += 2;
    d->ProgressBar->setValue(d->ProgressValue);
  }
  else if (d->CheckComplete)
  {
    if (d->ProgressValue < 100)
    {
      d->ProgressValue += 5;
      d->ProgressBar->setValue(qMin(d->ProgressValue, 100));
    }
    else
    {
      d->ProgressTimer->stop();
      if (d->CheckSuccess)
      {
        accept();
      }
    }
  }
}

//-----------------------------------------------------------------------------
void qLicenseCheckDialog::onLicenseCheckComplete(bool success)
{
  Q_D(qLicenseCheckDialog);

  d->CheckComplete = true;
  d->CheckSuccess = success;

  if (success)
  {
    d->StatusLabel->setText(QString::fromUtf8("授权验证成功，正在启动..."));
    d->StatusLabel->setStyleSheet(
      "QLabel {"
      "  color: #2ecc71;"
      "  font-size: 13px;"
      "  background: transparent;"
      "}"
    );
  }
  else
  {
    d->ProgressTimer->stop();
    d->ProgressBar->setValue(100);
    d->StatusLabel->setText(LicenseManager::instance().getLicenseInfo());
    d->StatusLabel->setStyleSheet(
      "QLabel {"
      "  color: #e74c3c;"
      "  font-size: 13px;"
      "  background: transparent;"
      "}"
    );
  }
}

