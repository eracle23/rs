/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#include "qChangePasswordDialog.h"
#include "UserManager.h"

// Qt includes
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QFont>

//-----------------------------------------------------------------------------
class qChangePasswordDialogPrivate
{
  Q_DECLARE_PUBLIC(qChangePasswordDialog)
protected:
  qChangePasswordDialog* const q_ptr;

public:
  qChangePasswordDialogPrivate(qChangePasswordDialog& object);
  void setupUi();

  QLabel* TitleLabel;
  QLabel* OldPasswordLabel;
  QLineEdit* OldPasswordEdit;
  QLabel* NewPasswordLabel;
  QLineEdit* NewPasswordEdit;
  QLabel* ConfirmPasswordLabel;
  QLineEdit* ConfirmPasswordEdit;
  QPushButton* ChangeButton;
  QPushButton* CancelButton;
};

//-----------------------------------------------------------------------------
qChangePasswordDialogPrivate::qChangePasswordDialogPrivate(qChangePasswordDialog& object)
  : q_ptr(&object)
  , TitleLabel(nullptr)
  , OldPasswordLabel(nullptr)
  , OldPasswordEdit(nullptr)
  , NewPasswordLabel(nullptr)
  , NewPasswordEdit(nullptr)
  , ConfirmPasswordLabel(nullptr)
  , ConfirmPasswordEdit(nullptr)
  , ChangeButton(nullptr)
  , CancelButton(nullptr)
{
}

//-----------------------------------------------------------------------------
void qChangePasswordDialogPrivate::setupUi()
{
  Q_Q(qChangePasswordDialog);

  q->setWindowTitle(QString::fromUtf8("修改密码"));
  q->setFixedSize(480, 450);
  q->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
  q->setAttribute(Qt::WA_TranslucentBackground, false);

  q->setStyleSheet(
    "qChangePasswordDialog {"
    "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
    "    stop:0 #1a237e, stop:1 #0d1421);"
    "  border: 2px solid #3498db;"
    "  border-radius: 10px;"
    "}"
  );

  QVBoxLayout* mainLayout = new QVBoxLayout(q);
  mainLayout->setContentsMargins(36, 32, 36, 32);
  mainLayout->setSpacing(12);

  TitleLabel = new QLabel(QString::fromUtf8("修改密码"));
  TitleLabel->setStyleSheet(
    "QLabel {"
    "  color: #ffffff;"
    "  font-size: 22px;"
    "  font-weight: bold;"
    "}"
  );
  TitleLabel->setAlignment(Qt::AlignCenter);

  OldPasswordLabel = new QLabel(QString::fromUtf8("原密码："));
  OldPasswordLabel->setStyleSheet(
    "QLabel {"
    "  color: #ffffff;"
    "  font-size: 14px;"
    "}"
  );

  OldPasswordEdit = new QLineEdit();
  OldPasswordEdit->setEchoMode(QLineEdit::Password);
  OldPasswordEdit->setPlaceholderText(QString::fromUtf8("请输入原密码"));
  OldPasswordEdit->setFixedHeight(34);
  OldPasswordEdit->setStyleSheet(
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
  );

  NewPasswordLabel = new QLabel(QString::fromUtf8("新密码："));
  NewPasswordLabel->setStyleSheet(
    "QLabel {"
    "  color: #ffffff;"
    "  font-size: 14px;"
    "}"
  );

  NewPasswordEdit = new QLineEdit();
  NewPasswordEdit->setEchoMode(QLineEdit::Password);
  NewPasswordEdit->setPlaceholderText(QString::fromUtf8("请输入新密码"));
  NewPasswordEdit->setFixedHeight(34);
  NewPasswordEdit->setStyleSheet(
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
  );

  ConfirmPasswordLabel = new QLabel(QString::fromUtf8("确认新密码："));
  ConfirmPasswordLabel->setStyleSheet(
    "QLabel {"
    "  color: #ffffff;"
    "  font-size: 14px;"
    "}"
  );

  ConfirmPasswordEdit = new QLineEdit();
  ConfirmPasswordEdit->setEchoMode(QLineEdit::Password);
  ConfirmPasswordEdit->setPlaceholderText(QString::fromUtf8("请再次输入新密码"));
  ConfirmPasswordEdit->setFixedHeight(34);
  ConfirmPasswordEdit->setStyleSheet(
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
  );

  ChangeButton = new QPushButton(QString::fromUtf8("修改"));
  ChangeButton->setStyleSheet(
    "QPushButton {"
    "  padding: 12px;"
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
  ChangeButton->setCursor(Qt::PointingHandCursor);

  CancelButton = new QPushButton(QString::fromUtf8("取消"));
  CancelButton->setStyleSheet(
    "QPushButton {"
    "  padding: 12px;"
    "  background: transparent;"
    "  color: #ffffff;"
    "  border: 1px solid #4a5568;"
    "  border-radius: 6px;"
    "  font-size: 16px;"
    "}"
    "QPushButton:hover {"
    "  background: #4a5568;"
    "}"
  );
  CancelButton->setCursor(Qt::PointingHandCursor);

  mainLayout->addWidget(TitleLabel);
  mainLayout->addSpacing(12);
  mainLayout->addWidget(OldPasswordLabel);
  mainLayout->addWidget(OldPasswordEdit);
  mainLayout->addSpacing(20);
  mainLayout->addWidget(NewPasswordLabel);
  mainLayout->addWidget(NewPasswordEdit);
  mainLayout->addSpacing(20);
  mainLayout->addWidget(ConfirmPasswordLabel);
  mainLayout->addWidget(ConfirmPasswordEdit);
  mainLayout->addSpacing(20);

  QHBoxLayout* buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();
  buttonLayout->addWidget(CancelButton);
  buttonLayout->addSpacing(10);
  buttonLayout->addWidget(ChangeButton);
  mainLayout->addLayout(buttonLayout);

  QObject::connect(ChangeButton, &QPushButton::clicked,
                   q, &qChangePasswordDialog::onChangePassword);
  QObject::connect(CancelButton, &QPushButton::clicked,
                   q, &QDialog::reject);

  OldPasswordEdit->setFocus();
}

//-----------------------------------------------------------------------------
qChangePasswordDialog::qChangePasswordDialog(QWidget* parent)
  : QDialog(parent)
  , d_ptr(new qChangePasswordDialogPrivate(*this))
{
  Q_D(qChangePasswordDialog);
  d->setupUi();
}

//-----------------------------------------------------------------------------
qChangePasswordDialog::~qChangePasswordDialog()
{
}

//-----------------------------------------------------------------------------
int qChangePasswordDialog::exec()
{
  Q_D(qChangePasswordDialog);
  return QDialog::exec();
}

//-----------------------------------------------------------------------------
void qChangePasswordDialog::onChangePassword()
{
  Q_D(qChangePasswordDialog);

  QString oldPassword = d->OldPasswordEdit->text();
  QString newPassword = d->NewPasswordEdit->text();
  QString confirmPassword = d->ConfirmPasswordEdit->text();

  if (oldPassword.isEmpty())
    {
      QMessageBox::warning(this, QString::fromUtf8("输入错误"),
                           QString::fromUtf8("请输入原密码"));
      d->OldPasswordEdit->setFocus();
      return;
    }

  if (newPassword.isEmpty())
    {
      QMessageBox::warning(this, QString::fromUtf8("输入错误"),
                           QString::fromUtf8("请输入新密码"));
      d->NewPasswordEdit->setFocus();
      return;
    }

  if (newPassword.length() < 6)
    {
      QMessageBox::warning(this, QString::fromUtf8("输入错误"),
                           QString::fromUtf8("新密码长度不能少于6位"));
      d->NewPasswordEdit->setFocus();
      return;
    }

  if (confirmPassword.isEmpty())
    {
      QMessageBox::warning(this, QString::fromUtf8("输入错误"),
                           QString::fromUtf8("请确认新密码"));
      d->ConfirmPasswordEdit->setFocus();
      return;
    }

  if (newPassword != confirmPassword)
    {
      QMessageBox::warning(this, QString::fromUtf8("输入错误"),
                           QString::fromUtf8("两次输入的新密码不一致"));
      d->ConfirmPasswordEdit->setFocus();
      return;
    }

  UserInfo currentUser = UserManager::instance().getCurrentUser();

  if (UserManager::instance().changePassword(currentUser.id, oldPassword, newPassword))
    {
      QMessageBox::information(this, QString::fromUtf8("成功"),
                               QString::fromUtf8("密码修改成功！"));
      accept();
    }
  else
    {
      QString error = UserManager::instance().getLastError();
      QMessageBox::critical(this, QString::fromUtf8("失败"), error);
    }
}
