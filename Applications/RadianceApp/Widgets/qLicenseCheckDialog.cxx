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
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QFont>
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QLinearGradient>
#include <QStackedWidget>
#include <QFrame>

//-----------------------------------------------------------------------------
class qLicenseCheckDialogPrivate
{
  Q_DECLARE_PUBLIC(qLicenseCheckDialog)
protected:
  qLicenseCheckDialog* const q_ptr;

public:
  qLicenseCheckDialogPrivate(qLicenseCheckDialog& object);
  void setupUi();
  void setupLoginPage(QWidget* page);
  void setupAuthPage(QWidget* page);

  QStackedWidget* Stack;
  QWidget* LoginPage;
  QLineEdit* UsernameEdit;
  QLineEdit* PasswordEdit;
  QPushButton* LoginButton;
  QLabel* LoginErrorLabel;
  QWidget* AuthPage;
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
  QString LoggedInUsername;
};

//-----------------------------------------------------------------------------
qLicenseCheckDialogPrivate::qLicenseCheckDialogPrivate(qLicenseCheckDialog& object)
  : q_ptr(&object)
  , Stack(nullptr)
  , LoginPage(nullptr)
  , UsernameEdit(nullptr)
  , PasswordEdit(nullptr)
  , LoginButton(nullptr)
  , LoginErrorLabel(nullptr)
  , AuthPage(nullptr)
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
static const char* s_commonLabelStyle =
  "QLabel { color: #ffffff; background: transparent; }";
static const char* s_subtitleStyle =
  "QLabel { color: #3498db; font-size: 14px; background: transparent; }";
static const char* s_lineEditStyle =
  "QLineEdit {"
  "  background: #34495e; color: #ecf0f1; border: 1px solid #3498db;"
  "  border-radius: 4px; padding: 8px; font-size: 13px; min-height: 20px;"
  "}"
  "QLineEdit:focus { border-color: #2ecc71; }";
static const char* s_primaryButtonStyle =
  "QPushButton {"
  "  background: #3498db; color: white; border: none; border-radius: 4px; font-size: 13px;"
  "}"
  "QPushButton:hover { background: #2980b9; }"
  "QPushButton:pressed { background: #21618c; }"
  "QPushButton:disabled { background: #7f8c8d; }";
static const char* s_exitButtonStyle =
  "QPushButton {"
  "  background: #e74c3c; color: white; border: none; border-radius: 4px; font-size: 13px;"
  "}"
  "QPushButton:hover { background: #c0392b; }"
  "QPushButton:pressed { background: #a93226; }";

//-----------------------------------------------------------------------------
void qLicenseCheckDialogPrivate::setupLoginPage(QWidget* page)
{
  Q_Q(qLicenseCheckDialog);
  QVBoxLayout* layout = new QVBoxLayout(page);
  layout->setContentsMargins(40, 30, 40, 30);
  layout->setSpacing(12);

  QLabel* title = new QLabel(QString::fromUtf8("医学影像三维重建软件"));
  title->setStyleSheet("QLabel { color: #ffffff; font-size: 22px; font-weight: bold; background: transparent; }");
  title->setAlignment(Qt::AlignCenter);
  layout->addWidget(title);

  QLabel* subtitle = new QLabel("Vision Magic Ecosystem");
  subtitle->setStyleSheet(s_subtitleStyle);
  subtitle->setAlignment(Qt::AlignCenter);
  layout->addWidget(subtitle);

  layout->addSpacing(16);

  QHBoxLayout* verLayout = new QHBoxLayout();
  verLayout->setSpacing(40);
  QLabel* vTitle1 = new QLabel(QString::fromUtf8("定期版本号"));
  vTitle1->setStyleSheet("QLabel { color: #95a5a6; font-size: 12px; background: transparent; }");
  QLabel* vVal1 = new QLabel(LicenseManager::getVersionString());
  vVal1->setStyleSheet("QLabel { color: #ffffff; font-size: 16px; font-weight: bold; background: transparent; }");
  QLabel* vTitle2 = new QLabel(QString::fromUtf8("发布版本号"));
  vTitle2->setStyleSheet("QLabel { color: #95a5a6; font-size: 12px; background: transparent; }");
  QLabel* vVal2 = new QLabel(LicenseManager::getReleaseVersion());
  vVal2->setStyleSheet("QLabel { color: #ffffff; font-size: 16px; font-weight: bold; background: transparent; }");
  verLayout->addStretch();
  verLayout->addWidget(vTitle1);
  verLayout->addWidget(vVal1);
  verLayout->addWidget(vTitle2);
  verLayout->addWidget(vVal2);
  verLayout->addStretch();
  layout->addLayout(verLayout);

  layout->addSpacing(24);

  QLabel* userLabel = new QLabel(QString::fromUtf8("账号"));
  userLabel->setStyleSheet(s_commonLabelStyle);
  UsernameEdit = new QLineEdit();
  UsernameEdit->setPlaceholderText(QString::fromUtf8("请输入用户名"));
  UsernameEdit->setStyleSheet(s_lineEditStyle);
  layout->addWidget(userLabel);
  layout->addWidget(UsernameEdit);

  QLabel* pwdLabel = new QLabel(QString::fromUtf8("密码"));
  pwdLabel->setStyleSheet(s_commonLabelStyle);
  PasswordEdit = new QLineEdit();
  PasswordEdit->setPlaceholderText(QString::fromUtf8("请输入密码"));
  PasswordEdit->setEchoMode(QLineEdit::Password);
  PasswordEdit->setStyleSheet(s_lineEditStyle);
  layout->addWidget(pwdLabel);
  layout->addWidget(PasswordEdit);

  LoginErrorLabel = new QLabel();
  LoginErrorLabel->setStyleSheet("QLabel { color: #e74c3c; font-size: 12px; background: transparent; }");
  LoginErrorLabel->setWordWrap(true);
  layout->addWidget(LoginErrorLabel);

  layout->addSpacing(12);

  QHBoxLayout* btnLayout = new QHBoxLayout();
  LoginButton = new QPushButton(QString::fromUtf8("登录"));
  LoginButton->setFixedSize(120, 36);
  LoginButton->setStyleSheet(s_primaryButtonStyle);
  QObject::connect(LoginButton, &QPushButton::clicked, q, &qLicenseCheckDialog::onLoginClicked);

  QPushButton* exitBtn = new QPushButton(QString::fromUtf8("退出"));
  exitBtn->setFixedSize(100, 36);
  exitBtn->setStyleSheet(s_exitButtonStyle);
  QObject::connect(exitBtn, &QPushButton::clicked, q, &QDialog::reject);

  btnLayout->addStretch();
  btnLayout->addWidget(LoginButton);
  btnLayout->addSpacing(12);
  btnLayout->addWidget(exitBtn);
  btnLayout->addStretch();
  layout->addLayout(btnLayout);
  layout->addStretch();
}

//-----------------------------------------------------------------------------
void qLicenseCheckDialogPrivate::setupAuthPage(QWidget* page)
{
  Q_Q(qLicenseCheckDialog);
  QVBoxLayout* mainLayout = new QVBoxLayout(page);
  mainLayout->setContentsMargins(40, 30, 40, 30);
  mainLayout->setSpacing(12);

  TitleLabel = new QLabel(QString::fromUtf8("医学影像三维重建软件"));
  TitleLabel->setStyleSheet("QLabel { color: #ffffff; font-size: 22px; font-weight: bold; background: transparent; }");
  TitleLabel->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(TitleLabel);

  SubtitleLabel = new QLabel("Vision Magic Ecosystem");
  SubtitleLabel->setStyleSheet(s_subtitleStyle);
  SubtitleLabel->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(SubtitleLabel);

  mainLayout->addSpacing(20);

  QHBoxLayout* versionLayout = new QHBoxLayout();
  versionLayout->setSpacing(40);
  QVBoxLayout* versionCol1 = new QVBoxLayout();
  QLabel* versionTitleLabel = new QLabel(QString::fromUtf8("定期版本号"));
  versionTitleLabel->setStyleSheet("QLabel { color: #95a5a6; font-size: 12px; background: transparent; }");
  VersionLabel = new QLabel(LicenseManager::getVersionString());
  VersionLabel->setStyleSheet("QLabel { color: #ffffff; font-size: 16px; font-weight: bold; background: transparent; }");
  versionCol1->addWidget(versionTitleLabel);
  versionCol1->addWidget(VersionLabel);
  QVBoxLayout* versionCol2 = new QVBoxLayout();
  QLabel* releaseTitleLabel = new QLabel(QString::fromUtf8("发布版本号"));
  releaseTitleLabel->setStyleSheet("QLabel { color: #95a5a6; font-size: 12px; background: transparent; }");
  ReleaseLabel = new QLabel(LicenseManager::getReleaseVersion());
  ReleaseLabel->setStyleSheet("QLabel { color: #ffffff; font-size: 16px; font-weight: bold; background: transparent; }");
  versionCol2->addWidget(releaseTitleLabel);
  versionCol2->addWidget(ReleaseLabel);
  versionLayout->addStretch();
  versionLayout->addLayout(versionCol1);
  versionLayout->addLayout(versionCol2);
  versionLayout->addStretch();
  mainLayout->addLayout(versionLayout);

  mainLayout->addSpacing(20);

  ProgressBar = new QProgressBar();
  ProgressBar->setRange(0, 100);
  ProgressBar->setValue(0);
  ProgressBar->setTextVisible(false);
  ProgressBar->setFixedHeight(8);
  ProgressBar->setStyleSheet(
    "QProgressBar { background: #34495e; border: none; border-radius: 4px; }"
    "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3498db, stop:1 #2ecc71); border-radius: 4px; }"
  );
  mainLayout->addWidget(ProgressBar);

  StatusLabel = new QLabel(QString::fromUtf8("正在验证授权..."));
  StatusLabel->setStyleSheet("QLabel { color: #bdc3c7; font-size: 13px; background: transparent; }");
  StatusLabel->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(StatusLabel);

  mainLayout->addStretch();

  QHBoxLayout* buttonLayout = new QHBoxLayout();
  ExitButton = new QPushButton(QString::fromUtf8("退出"));
  ExitButton->setFixedSize(100, 32);
  ExitButton->setStyleSheet(s_exitButtonStyle);
  QObject::connect(ExitButton, &QPushButton::clicked, q, &QDialog::reject);
  buttonLayout->addStretch();
  buttonLayout->addWidget(ExitButton);
  buttonLayout->addStretch();
  mainLayout->addLayout(buttonLayout);

  ProgressTimer = new QTimer(q);
  QObject::connect(ProgressTimer, &QTimer::timeout, q, &qLicenseCheckDialog::updateProgress);
}

//-----------------------------------------------------------------------------
void qLicenseCheckDialogPrivate::setupUi()
{
  Q_Q(qLicenseCheckDialog);

  q->setWindowTitle(QString::fromUtf8("登录"));
  q->setFixedSize(480, 420);
  q->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
  q->setAttribute(Qt::WA_TranslucentBackground, false);

  q->setStyleSheet(
    "qLicenseCheckDialog {"
    "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
    "    stop:0 #2c3e50, stop:1 #1a252f);"
    "  border: 2px solid #3498db;"
    "  border-radius: 10px;"
    "}"
  );

  Stack = new QStackedWidget(q);
  LoginPage = new QWidget();
  AuthPage = new QWidget();
  setupLoginPage(LoginPage);
  setupAuthPage(AuthPage);
  Stack->addWidget(LoginPage);
  Stack->addWidget(AuthPage);
  Stack->setCurrentWidget(LoginPage);

  QVBoxLayout* mainLayout = new QVBoxLayout(q);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->addWidget(Stack);
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
QString qLicenseCheckDialog::loggedInUsername() const
{
  Q_D(const qLicenseCheckDialog);
  return d->LoggedInUsername;
}

//-----------------------------------------------------------------------------
void qLicenseCheckDialog::onLoginClicked()
{
  Q_D(qLicenseCheckDialog);

  QString user = d->UsernameEdit->text().trimmed();
  QString pwd = d->PasswordEdit->text();

  d->LoginErrorLabel->clear();

  if (user.isEmpty())
  {
    d->LoginErrorLabel->setText(QString::fromUtf8("请输入用户名"));
    return;
  }
  if (pwd.isEmpty())
  {
    d->LoginErrorLabel->setText(QString::fromUtf8("请输入密码"));
    return;
  }

  // TODO: 在此处接入实际校验（本地校验或请求服务器）
  // 示例：若需对接后端，可调用 QNetworkAccessManager 等，在回调中切换页面或显示错误
  // 当前为本地占位校验：仅要求非空即通过
  bool loginOk = true;
  if (!loginOk)
  {
    d->LoginErrorLabel->setText(QString::fromUtf8("用户名或密码错误"));
    return;
  }

  d->LoggedInUsername = user;
  d->Stack->setCurrentWidget(d->AuthPage);
  QApplication::processEvents();
  QTimer::singleShot(100, this, &qLicenseCheckDialog::startLicenseCheck);
}

//-----------------------------------------------------------------------------
int qLicenseCheckDialog::exec()
{
  Q_D(qLicenseCheckDialog);

  // 每次弹窗都强制先显示登录页，避免被误切到授权页
  d->Stack->setCurrentWidget(d->LoginPage);
  d->LoginErrorLabel->clear();
  show();
  QApplication::processEvents();
  // 用户点击登录后由 onLoginClicked 切换到授权页并启动 startLicenseCheck
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

