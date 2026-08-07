/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#include "qLoginDialog.h"
#include "UserManager.h"

// Qt includes
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QTimer>
#include <QFont>
#include <QSizePolicy>
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QLinearGradient>
#include <QMouseEvent>

//-----------------------------------------------------------------------------
class qLoginDialogPrivate
{
  Q_DECLARE_PUBLIC(qLoginDialog)
protected:
  qLoginDialog* const q_ptr;

public:
  qLoginDialogPrivate(qLoginDialog& object);
  void setupUi();

  QPoint dragStartPosition;
  bool dragging = false;

  QLabel* TitleLabel;
  QLabel* SubtitleLabel;
  QLabel* UsernameLabel;
  QLineEdit* UsernameEdit;
  QLabel* PasswordLabel;
  QLineEdit* PasswordEdit;
  QPushButton* LoginButton;
  QPushButton* ForgotPasswordButton;
  QPushButton* ExitButton;
};

//-----------------------------------------------------------------------------
qLoginDialogPrivate::qLoginDialogPrivate(qLoginDialog& object)
  : q_ptr(&object)
  , TitleLabel(nullptr)
  , SubtitleLabel(nullptr)
  , UsernameLabel(nullptr)
  , UsernameEdit(nullptr)
  , PasswordLabel(nullptr)
  , PasswordEdit(nullptr)
  , LoginButton(nullptr)
  , ForgotPasswordButton(nullptr)
  , ExitButton(nullptr)
{
}

//-----------------------------------------------------------------------------
void qLoginDialogPrivate::setupUi()
{
  Q_Q(qLoginDialog);

  q->setWindowTitle(QString::fromUtf8("用户登录"));
  q->setFixedSize(500, 460);
  q->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
  q->setAttribute(Qt::WA_TranslucentBackground, false);

  q->setStyleSheet(
    "qLoginDialog {"
    "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
    "    stop:0 #1a237e, stop:1 #0d1421);"
    "  border: 2px solid #3498db;"
    "  border-radius: 10px;"
    "}"
  );

  QVBoxLayout* mainLayout = new QVBoxLayout(q);
  mainLayout->setContentsMargins(36, 32, 36, 32);
  mainLayout->setSpacing(12);

  TitleLabel = new QLabel(QString::fromUtf8("医学影像三维重建软件"));
  TitleLabel->setStyleSheet(
    "QLabel {"
    "  color: #ffffff;"
    "  font-size: 22px;"
    "  font-weight: bold;"
    "}"
  );
  TitleLabel->setAlignment(Qt::AlignCenter);
  TitleLabel->setMinimumWidth(300);
  TitleLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
  TitleLabel->setWordWrap(false);
  TitleLabel->setCursor(Qt::SizeAllCursor);
  TitleLabel->installEventFilter(q);

  SubtitleLabel = new QLabel(QString::fromUtf8("请登录以继续使用软件"));
  SubtitleLabel->setStyleSheet(
    "QLabel {"
    "  color: #a0c4e8;"
    "  font-size: 14px;"
    "}"
  );
  SubtitleLabel->setAlignment(Qt::AlignCenter);
  SubtitleLabel->setMinimumWidth(200);
  SubtitleLabel->setWordWrap(false);
  SubtitleLabel->setCursor(Qt::SizeAllCursor);
  SubtitleLabel->installEventFilter(q);

  UsernameLabel = new QLabel(QString::fromUtf8("用户名："));
  UsernameLabel->setStyleSheet(
    "QLabel {"
    "  color: #ffffff;"
    "  font-size: 14px;"
    "}"
  );
  UsernameLabel->setMinimumWidth(80);
  UsernameLabel->setWordWrap(false);

  UsernameEdit = new QLineEdit();
  UsernameEdit->setPlaceholderText(QString::fromUtf8("请输入用户名"));
  UsernameEdit->setFixedHeight(34);
  UsernameEdit->setStyleSheet(
    "QLineEdit {"
    "  padding: 6px 12px;"
    "  border: 1px solid #4a5568;"
    "  border-radius: 6px;"
    "  background: #ffffff;"
    "  color: #333333;"
    "  font-size: 14px;"
    "}"
    "QLineEdit:focus {"
    "  border: 2px solid #3498db;"
    "}"
    "QLineEdit::placeholder {"
    "  color: #888888;"
    "}"
  );

  PasswordLabel = new QLabel(QString::fromUtf8("密码："));
  PasswordLabel->setStyleSheet(
    "QLabel {"
    "  color: #ffffff;"
    "  font-size: 14px;"
    "}"
  );
  PasswordLabel->setMinimumWidth(80);
  PasswordLabel->setWordWrap(false);

  PasswordEdit = new QLineEdit();
  PasswordEdit->setEchoMode(QLineEdit::Password);
  PasswordEdit->setPlaceholderText(QString::fromUtf8("请输入密码"));
  PasswordEdit->setFixedHeight(34);
  PasswordEdit->setStyleSheet(
    "QLineEdit {"
    "  padding: 6px 12px;"
    "  border: 1px solid #4a5568;"
    "  border-radius: 6px;"
    "  background: #ffffff;"
    "  color: #333333;"
    "  font-size: 14px;"
    "}"
    "QLineEdit:focus {"
    "  border: 2px solid #3498db;"
    "}"
    "QLineEdit::placeholder {"
    "  color: #888888;"
    "}"
  );

  LoginButton = new QPushButton(QString::fromUtf8("登录"));
  LoginButton->setStyleSheet(
    "QPushButton {"
    "  padding: 10px 24px;"
    "  min-width: 120px;"
    "  min-height: 38px;"
    "  background: #3498db;"
    "  color: #ffffff;"
    "  border: none;"
    "  border-radius: 6px;"
    "  font-size: 16px;"
    "  font-weight: bold;"
    "}"
    "QPushButton:hover {"
    "  background: #2980b9;"
    "}"
    "QPushButton:pressed {"
    "  background: #1e5a9e;"
    "}"
  );
  LoginButton->setCursor(Qt::PointingHandCursor);
  LoginButton->setMinimumWidth(120);
  LoginButton->setFixedHeight(38);
  LoginButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

  ForgotPasswordButton = new QPushButton(QString::fromUtf8("忘记密码？"));
  ForgotPasswordButton->setStyleSheet(
    "QPushButton {"
    "  background: transparent;"
    "  color: #a0c4e8;"
    "  border: none;"
    "  font-size: 13px;"
    "  text-decoration: underline;"
    "}"
    "QPushButton:hover {"
    "  color: #3498db;"
    "}"
  );
  ForgotPasswordButton->setCursor(Qt::PointingHandCursor);
  ForgotPasswordButton->setMinimumWidth(90);

  ExitButton = new QPushButton(QString::fromUtf8("退出"));
  ExitButton->setStyleSheet(
    "QPushButton {"
    "  padding: 8px 16px;"
    "  background: transparent;"
    "  color: #ffffff;"
    "  border: 1px solid #4a5568;"
    "  border-radius: 6px;"
    "  font-size: 14px;"
    "}"
    "QPushButton:hover {"
    "  background: #4a5568;"
    "}"
  );
  ExitButton->setCursor(Qt::PointingHandCursor);
  ExitButton->setMinimumWidth(70);

  mainLayout->addWidget(TitleLabel);
  mainLayout->addWidget(SubtitleLabel);
  mainLayout->addSpacing(12);
  mainLayout->addWidget(UsernameLabel);
  mainLayout->addWidget(UsernameEdit);
  mainLayout->addSpacing(20);
  mainLayout->addWidget(PasswordLabel);
  mainLayout->addWidget(PasswordEdit);
  mainLayout->addSpacing(20);
  mainLayout->addWidget(LoginButton);
  mainLayout->addSpacing(8);

  QHBoxLayout* buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();
  buttonLayout->addWidget(ForgotPasswordButton);
  buttonLayout->addWidget(ExitButton);
  mainLayout->addLayout(buttonLayout);

  QObject::connect(UsernameEdit, &QLineEdit::textChanged,
                   q, &qLoginDialog::onUsernameChanged);
  QObject::connect(PasswordEdit, &QLineEdit::textChanged,
                   q, &qLoginDialog::onPasswordChanged);
  QObject::connect(LoginButton, &QPushButton::clicked,
                   q, &qLoginDialog::onLoginClicked);
  QObject::connect(ForgotPasswordButton, &QPushButton::clicked,
                   q, &qLoginDialog::onForgotPassword);
  QObject::connect(ExitButton, &QPushButton::clicked,
                   q, &QDialog::reject);

  UsernameEdit->setFocus();
}

//-----------------------------------------------------------------------------
qLoginDialog::qLoginDialog(QWidget* parent)
  : QDialog(parent)
  , d_ptr(new qLoginDialogPrivate(*this))
{
  Q_D(qLoginDialog);
  d->setupUi();
}

//-----------------------------------------------------------------------------
qLoginDialog::~qLoginDialog()
{
}

//-----------------------------------------------------------------------------
bool qLoginDialog::eventFilter(QObject* obj, QEvent* event)
{
  Q_D(qLoginDialog);
  if (obj != d->TitleLabel && obj != d->SubtitleLabel)
  {
    return QDialog::eventFilter(obj, event);
  }

  switch (event->type())
  {
    case QEvent::MouseButtonPress:
    {
      QMouseEvent* me = static_cast<QMouseEvent*>(event);
      if (me->button() == Qt::LeftButton)
      {
        d->dragStartPosition = me->globalPos() - frameGeometry().topLeft();
        d->dragging = true;
      }
      break;
    }
    case QEvent::MouseMove:
    {
      if (d->dragging)
      {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        move(me->globalPos() - d->dragStartPosition);
      }
      break;
    }
    case QEvent::MouseButtonRelease:
    {
      QMouseEvent* me = static_cast<QMouseEvent*>(event);
      if (me->button() == Qt::LeftButton)
      {
        d->dragging = false;
      }
      break;
    }
    default:
      break;
  }
  return QDialog::eventFilter(obj, event);
}

//-----------------------------------------------------------------------------
int qLoginDialog::exec()
{
  Q_D(qLoginDialog);
  return QDialog::exec();
}

//-----------------------------------------------------------------------------
void qLoginDialog::onLoginClicked()
{
  Q_D(qLoginDialog);

  QString username = d->UsernameEdit->text().trimmed();
  QString password = d->PasswordEdit->text();

  if (username.isEmpty())
    {
      QMessageBox::warning(this, QString::fromUtf8("登录失败"),
                           QString::fromUtf8("请输入用户名"));
      d->UsernameEdit->setFocus();
      return;
    }

  if (password.isEmpty())
    {
      QMessageBox::warning(this, QString::fromUtf8("登录失败"),
                           QString::fromUtf8("请输入密码"));
      d->PasswordEdit->setFocus();
      return;
    }

  d->LoginButton->setEnabled(false);
  d->LoginButton->setText(QString::fromUtf8("登录中..."));

  QTimer::singleShot(500, [this, d, username, password]()
    {
      if (UserManager::instance().authenticate(username, password))
        {
          UserInfo user = UserManager::instance().getCurrentUser();
          QString message = QString::fromUtf8("登录成功！\n\n欢迎回来，%1")
            .arg(user.fullName);
          QMessageBox::information(this, QString::fromUtf8("登录成功"), message);
          accept();
        }
      else
        {
          QString error = UserManager::instance().getLastError();
          QMessageBox::critical(this, QString::fromUtf8("登录失败"), error);
          d->PasswordEdit->clear();
          d->PasswordEdit->setFocus();
        }

      d->LoginButton->setEnabled(true);
      d->LoginButton->setText(QString::fromUtf8("登录"));
    });
}

//-----------------------------------------------------------------------------
void qLoginDialog::onUsernameChanged(const QString& text)
{
  Q_D(qLoginDialog);
  bool hasText = !text.trimmed().isEmpty();
  d->LoginButton->setEnabled(hasText && !d->PasswordEdit->text().isEmpty());
}

//-----------------------------------------------------------------------------
void qLoginDialog::onPasswordChanged(const QString& text)
{
  Q_D(qLoginDialog);
  bool hasText = !text.isEmpty();
  d->LoginButton->setEnabled(hasText && !d->UsernameEdit->text().trimmed().isEmpty());
}

//-----------------------------------------------------------------------------
void qLoginDialog::onForgotPassword()
{
  QMessageBox::information(this, QString::fromUtf8("忘记密码"),
                       QString::fromUtf8("请联系管理员重置密码。\n\n管理员邮箱：admin@radiancelabs.com"));
}
