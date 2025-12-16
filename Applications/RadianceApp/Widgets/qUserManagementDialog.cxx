/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#include "qUserManagementDialog.h"
#include "UserManager.h"

// Qt includes
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QHeaderView>
#include <QGroupBox>

//-----------------------------------------------------------------------------
class qUserManagementDialogPrivate
{
  Q_DECLARE_PUBLIC(qUserManagementDialog)
protected:
  qUserManagementDialog* const q_ptr;

public:
  qUserManagementDialogPrivate(qUserManagementDialog& object);
  void setupUi();
  void refreshTable();
  int getSelectedUserId() const;
  UserInfo getSelectedUser() const;

  QLabel* TitleLabel;
  QTableWidget* UserTable;
  QPushButton* AddButton;
  QPushButton* EditButton;
  QPushButton* DeleteButton;
  QPushButton* LockButton;
  QPushButton* UnlockButton;
  QPushButton* ResetPasswordButton;
  QPushButton* RefreshButton;
  QPushButton* CloseButton;
};

//-----------------------------------------------------------------------------
qUserManagementDialogPrivate::qUserManagementDialogPrivate(qUserManagementDialog& object)
  : q_ptr(&object)
  , TitleLabel(nullptr)
  , UserTable(nullptr)
  , AddButton(nullptr)
  , EditButton(nullptr)
  , DeleteButton(nullptr)
  , LockButton(nullptr)
  , UnlockButton(nullptr)
  , ResetPasswordButton(nullptr)
  , RefreshButton(nullptr)
  , CloseButton(nullptr)
{
}

//-----------------------------------------------------------------------------
int qUserManagementDialogPrivate::getSelectedUserId() const
{
  if (!UserTable || UserTable->currentRow() < 0)
    {
      return -1;
    }
  QTableWidgetItem* idItem = UserTable->item(UserTable->currentRow(), 0);
  if (!idItem)
    {
      return -1;
    }
  return idItem->data(Qt::UserRole).toInt();
}

//-----------------------------------------------------------------------------
UserInfo qUserManagementDialogPrivate::getSelectedUser() const
{
  int userId = getSelectedUserId();
  if (userId < 0)
    {
      return UserInfo();
    }
  return UserManager::instance().getUserById(userId);
}

//-----------------------------------------------------------------------------
void qUserManagementDialogPrivate::refreshTable()
{
  if (!UserTable)
    {
      return;
    }
  UserTable->setRowCount(0);
  QVector<UserInfo> users = UserManager::instance().getAllUsers();
  for (const UserInfo& user : users)
    {
      int row = UserTable->rowCount();
      UserTable->insertRow(row);

      QTableWidgetItem* idItem = new QTableWidgetItem(QString::number(user.id));
      idItem->setData(Qt::UserRole, user.id);
      idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
      UserTable->setItem(row, 0, idItem);

      QTableWidgetItem* usernameItem = new QTableWidgetItem(user.username);
      usernameItem->setFlags(usernameItem->flags() & ~Qt::ItemIsEditable);
      UserTable->setItem(row, 1, usernameItem);

      QTableWidgetItem* roleItem = new QTableWidgetItem(user.role);
      roleItem->setFlags(roleItem->flags() & ~Qt::ItemIsEditable);
      UserTable->setItem(row, 2, roleItem);

      QTableWidgetItem* fullNameItem = new QTableWidgetItem(user.fullName);
      fullNameItem->setFlags(fullNameItem->flags() & ~Qt::ItemIsEditable);
      UserTable->setItem(row, 3, fullNameItem);

      QTableWidgetItem* emailItem = new QTableWidgetItem(user.email);
      emailItem->setFlags(emailItem->flags() & ~Qt::ItemIsEditable);
      UserTable->setItem(row, 4, emailItem);

      QString status;
      if (user.isLocked)
        {
          status = QString::fromUtf8("已锁定");
        }
      else if (user.isActive)
        {
          status = QString::fromUtf8("正常");
        }
      else
        {
          status = QString::fromUtf8("已禁用");
        }
      QTableWidgetItem* statusItem = new QTableWidgetItem(status);
      statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
      UserTable->setItem(row, 5, statusItem);
    }
}

//-----------------------------------------------------------------------------
void qUserManagementDialogPrivate::setupUi()
{
  Q_Q(qUserManagementDialog);

  q->setWindowTitle(QString::fromUtf8("用户管理"));
  q->setMinimumSize(800, 500);
  q->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
  q->setAttribute(Qt::WA_TranslucentBackground, false);

  q->setStyleSheet(
    "qUserManagementDialog {"
    "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
    "    stop:0 #1a237e, stop:1 #0d1421);"
    "  border: 2px solid #3498db;"
    "  border-radius: 10px;"
    "}"
  );

  QVBoxLayout* mainLayout = new QVBoxLayout(q);
  mainLayout->setContentsMargins(24, 24, 24, 24);
  mainLayout->setSpacing(12);

  TitleLabel = new QLabel(QString::fromUtf8("用户管理"));
  TitleLabel->setStyleSheet(
    "QLabel {"
    "  color: #ffffff;"
    "  font-size: 20px;"
    "  font-weight: bold;"
    "}"
  );
  TitleLabel->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(TitleLabel);

  UserTable = new QTableWidget(0, 6);
  UserTable->setHorizontalHeaderLabels(QStringList()
    << QString::fromUtf8("ID")
    << QString::fromUtf8("用户名")
    << QString::fromUtf8("角色")
    << QString::fromUtf8("姓名")
    << QString::fromUtf8("邮箱")
    << QString::fromUtf8("状态"));
  UserTable->horizontalHeader()->setStretchLastSection(true);
  UserTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  UserTable->setSelectionMode(QAbstractItemView::SingleSelection);
  UserTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  UserTable->verticalHeader()->setVisible(false);
  UserTable->setColumnHidden(0, true);
  UserTable->setStyleSheet(
    "QTableWidget {"
    "  background: #1e293b;"
    "  color: #e2e8f0;"
    "  gridline-color: #334155;"
    "  border: 1px solid #475569;"
    "  border-radius: 6px;"
    "}"
    "QTableWidget::item {"
    "  padding: 6px;"
    "}"
    "QTableWidget::item:selected {"
    "  background: #3498db;"
    "  color: #ffffff;"
    "}"
    "QHeaderView::section {"
    "  background: #0f172a;"
    "  color: #94a3b8;"
    "  padding: 8px;"
    "  font-weight: bold;"
    "  border: none;"
    "  border-bottom: 2px solid #3498db;"
    "}"
  );
  mainLayout->addWidget(UserTable);

  QString buttonStyle =
    "QPushButton {"
    "  padding: 8px 16px;"
    "  min-width: 80px;"
    "  background: #3498db;"
    "  color: #ffffff;"
    "  border: none;"
    "  border-radius: 6px;"
    "  font-size: 14px;"
    "}"
    "QPushButton:hover {"
    "  background: #2980b9;"
    "}"
    "QPushButton:pressed {"
    "  background: #1e5a9e;"
    "}"
    "QPushButton:disabled {"
    "  background: #4a5568;"
    "  color: #94a3b8;"
    "}";

  QString secondaryButtonStyle =
    "QPushButton {"
    "  padding: 8px 16px;"
    "  min-width: 80px;"
    "  background: transparent;"
    "  color: #ffffff;"
    "  border: 1px solid #4a5568;"
    "  border-radius: 6px;"
    "  font-size: 14px;"
    "}"
    "QPushButton:hover {"
    "  background: #4a5568;"
    "}";

  QHBoxLayout* buttonLayout = new QHBoxLayout();
  buttonLayout->setSpacing(8);

  AddButton = new QPushButton(QString::fromUtf8("添加"));
  AddButton->setStyleSheet(buttonStyle);
  AddButton->setCursor(Qt::PointingHandCursor);

  EditButton = new QPushButton(QString::fromUtf8("编辑"));
  EditButton->setStyleSheet(buttonStyle);
  EditButton->setCursor(Qt::PointingHandCursor);
  EditButton->setEnabled(false);

  DeleteButton = new QPushButton(QString::fromUtf8("删除"));
  DeleteButton->setStyleSheet(buttonStyle);
  DeleteButton->setCursor(Qt::PointingHandCursor);
  DeleteButton->setEnabled(false);

  LockButton = new QPushButton(QString::fromUtf8("锁定"));
  LockButton->setStyleSheet(buttonStyle);
  LockButton->setCursor(Qt::PointingHandCursor);
  LockButton->setEnabled(false);

  UnlockButton = new QPushButton(QString::fromUtf8("解锁"));
  UnlockButton->setStyleSheet(buttonStyle);
  UnlockButton->setCursor(Qt::PointingHandCursor);
  UnlockButton->setEnabled(false);

  ResetPasswordButton = new QPushButton(QString::fromUtf8("重置密码"));
  ResetPasswordButton->setStyleSheet(buttonStyle);
  ResetPasswordButton->setCursor(Qt::PointingHandCursor);
  ResetPasswordButton->setEnabled(false);

  RefreshButton = new QPushButton(QString::fromUtf8("刷新"));
  RefreshButton->setStyleSheet(secondaryButtonStyle);
  RefreshButton->setCursor(Qt::PointingHandCursor);

  CloseButton = new QPushButton(QString::fromUtf8("关闭"));
  CloseButton->setStyleSheet(secondaryButtonStyle);
  CloseButton->setCursor(Qt::PointingHandCursor);

  buttonLayout->addWidget(AddButton);
  buttonLayout->addWidget(EditButton);
  buttonLayout->addWidget(DeleteButton);
  buttonLayout->addWidget(LockButton);
  buttonLayout->addWidget(UnlockButton);
  buttonLayout->addWidget(ResetPasswordButton);
  buttonLayout->addStretch();
  buttonLayout->addWidget(RefreshButton);
  buttonLayout->addWidget(CloseButton);
  mainLayout->addLayout(buttonLayout);

  QObject::connect(AddButton, &QPushButton::clicked,
                   q, &qUserManagementDialog::onAddUser);
  QObject::connect(EditButton, &QPushButton::clicked,
                   q, &qUserManagementDialog::onEditUser);
  QObject::connect(DeleteButton, &QPushButton::clicked,
                   q, &qUserManagementDialog::onDeleteUser);
  QObject::connect(LockButton, &QPushButton::clicked,
                   q, &qUserManagementDialog::onLockUser);
  QObject::connect(UnlockButton, &QPushButton::clicked,
                   q, &qUserManagementDialog::onUnlockUser);
  QObject::connect(ResetPasswordButton, &QPushButton::clicked,
                   q, &qUserManagementDialog::onResetPassword);
  QObject::connect(RefreshButton, &QPushButton::clicked,
                   q, &qUserManagementDialog::onRefresh);
  QObject::connect(CloseButton, &QPushButton::clicked,
                   q, &QDialog::accept);
  QObject::connect(UserTable->selectionModel(), &QItemSelectionModel::selectionChanged,
                   q, &qUserManagementDialog::onUserSelectionChanged);
  QObject::connect(UserTable, &QTableWidget::itemDoubleClicked,
                   q, &qUserManagementDialog::onUserDoubleClicked);

  refreshTable();
}

//-----------------------------------------------------------------------------
qUserManagementDialog::qUserManagementDialog(QWidget* parent)
  : QDialog(parent)
  , d_ptr(new qUserManagementDialogPrivate(*this))
{
  Q_D(qUserManagementDialog);
  d->setupUi();
}

//-----------------------------------------------------------------------------
qUserManagementDialog::~qUserManagementDialog()
{
}

//-----------------------------------------------------------------------------
int qUserManagementDialog::exec()
{
  Q_D(qUserManagementDialog);
  d->refreshTable();
  return QDialog::exec();
}

//-----------------------------------------------------------------------------
void qUserManagementDialog::onAddUser()
{
  Q_D(qUserManagementDialog);

  UserInfo currentUser = UserManager::instance().getCurrentUser();
  if (currentUser.role != QString::fromUtf8("admin") && currentUser.role != "admin")
    {
      QMessageBox::warning(this, QString::fromUtf8("权限不足"),
                           QString::fromUtf8("仅管理员可以添加用户"));
      return;
    }

  QString username = QInputDialog::getText(this, QString::fromUtf8("添加用户"),
    QString::fromUtf8("用户名："), QLineEdit::Normal);
  if (username.isEmpty())
    {
      return;
    }
  username = username.trimmed();

  QString password = QInputDialog::getText(this, QString::fromUtf8("添加用户"),
    QString::fromUtf8("初始密码："), QLineEdit::Password);
  if (password.isEmpty())
    {
      return;
    }
  if (password.length() < 6)
    {
      QMessageBox::warning(this, QString::fromUtf8("输入错误"),
                           QString::fromUtf8("密码长度不能少于6位"));
      return;
    }

  QString role = QInputDialog::getItem(this, QString::fromUtf8("添加用户"),
    QString::fromUtf8("角色："),
    QStringList() << "admin" << "user",
    1);
  if (role.isEmpty())
    {
      return;
    }

  UserInfo newUser;
  newUser.id = -1;
  newUser.username = username;
  newUser.passwordHash = UserManager::instance().hashPassword(password);
  newUser.role = role;
  newUser.fullName = username;
  newUser.email = QString();
  newUser.createdAt = QDateTime::currentDateTime();
  newUser.isActive = true;
  newUser.isLocked = false;
  newUser.failedAttempts = 0;

  if (UserManager::instance().addUser(newUser))
    {
      QMessageBox::information(this, QString::fromUtf8("成功"),
                               QString::fromUtf8("用户添加成功"));
      d->refreshTable();
    }
  else
    {
      QMessageBox::critical(this, QString::fromUtf8("失败"),
                            UserManager::instance().getLastError());
    }
}

//-----------------------------------------------------------------------------
void qUserManagementDialog::onEditUser()
{
  Q_D(qUserManagementDialog);

  UserInfo user = d->getSelectedUser();
  if (user.id < 0)
    {
      QMessageBox::warning(this, QString::fromUtf8("提示"),
                           QString::fromUtf8("请先选择要编辑的用户"));
      return;
    }

  UserInfo currentUser = UserManager::instance().getCurrentUser();
  if (currentUser.role != QString::fromUtf8("admin") && currentUser.role != "admin")
    {
      QMessageBox::warning(this, QString::fromUtf8("权限不足"),
                           QString::fromUtf8("仅管理员可以编辑用户"));
      return;
    }

  QString fullName = QInputDialog::getText(this, QString::fromUtf8("编辑用户"),
    QString::fromUtf8("姓名："), QLineEdit::Normal, user.fullName);
  if (fullName.isNull())
    {
      return;
    }

  QString email = QInputDialog::getText(this, QString::fromUtf8("编辑用户"),
    QString::fromUtf8("邮箱："), QLineEdit::Normal, user.email);
  if (email.isNull())
    {
      return;
    }

  QString role = QInputDialog::getItem(this, QString::fromUtf8("编辑用户"),
    QString::fromUtf8("角色："),
    QStringList() << "admin" << "user",
    user.role == "admin" ? 0 : 1);
  if (role.isEmpty())
    {
      return;
    }

  user.fullName = fullName.trimmed();
  user.email = email.trimmed();
  user.role = role;

  if (UserManager::instance().updateUser(user))
    {
      QMessageBox::information(this, QString::fromUtf8("成功"),
                               QString::fromUtf8("用户信息已更新"));
      d->refreshTable();
    }
  else
    {
      QMessageBox::critical(this, QString::fromUtf8("失败"),
                            UserManager::instance().getLastError());
    }
}

//-----------------------------------------------------------------------------
void qUserManagementDialog::onDeleteUser()
{
  Q_D(qUserManagementDialog);

  UserInfo user = d->getSelectedUser();
  if (user.id < 0)
    {
      QMessageBox::warning(this, QString::fromUtf8("提示"),
                           QString::fromUtf8("请先选择要删除的用户"));
      return;
    }

  UserInfo currentUser = UserManager::instance().getCurrentUser();
  if (currentUser.role != QString::fromUtf8("admin") && currentUser.role != "admin")
    {
      QMessageBox::warning(this, QString::fromUtf8("权限不足"),
                           QString::fromUtf8("仅管理员可以删除用户"));
      return;
    }

  if (user.id == currentUser.id)
    {
      QMessageBox::warning(this, QString::fromUtf8("提示"),
                           QString::fromUtf8("不能删除当前登录的用户"));
      return;
    }

  auto reply = QMessageBox::question(this, QString::fromUtf8("确认删除"),
    QString::fromUtf8("确定要删除用户 \"%1\" 吗？此操作不可恢复。")
      .arg(user.username),
    QMessageBox::Yes | QMessageBox::No,
    QMessageBox::No);

  if (reply != QMessageBox::Yes)
    {
      return;
    }

  if (UserManager::instance().deleteUser(user.id))
    {
      QMessageBox::information(this, QString::fromUtf8("成功"),
                               QString::fromUtf8("用户已删除"));
      d->refreshTable();
    }
  else
    {
      QMessageBox::critical(this, QString::fromUtf8("失败"),
                            UserManager::instance().getLastError());
    }
}

//-----------------------------------------------------------------------------
void qUserManagementDialog::onLockUser()
{
  Q_D(qUserManagementDialog);

  int userId = d->getSelectedUserId();
  if (userId < 0)
    {
      QMessageBox::warning(this, QString::fromUtf8("提示"),
                           QString::fromUtf8("请先选择要锁定的用户"));
      return;
    }

  UserInfo currentUser = UserManager::instance().getCurrentUser();
  if (currentUser.role != QString::fromUtf8("admin") && currentUser.role != "admin")
    {
      QMessageBox::warning(this, QString::fromUtf8("权限不足"),
                           QString::fromUtf8("仅管理员可以锁定用户"));
      return;
    }

  if (userId == currentUser.id)
    {
      QMessageBox::warning(this, QString::fromUtf8("提示"),
                           QString::fromUtf8("不能锁定当前登录的用户"));
      return;
    }

  UserInfo user = UserManager::instance().getUserById(userId);
  if (user.isLocked)
    {
      QMessageBox::information(this, QString::fromUtf8("提示"),
                               QString::fromUtf8("该用户已处于锁定状态"));
      return;
    }

  if (UserManager::instance().lockUser(userId))
    {
      QMessageBox::information(this, QString::fromUtf8("成功"),
                               QString::fromUtf8("用户已锁定"));
      d->refreshTable();
    }
  else
    {
      QMessageBox::critical(this, QString::fromUtf8("失败"),
                            UserManager::instance().getLastError());
    }
}

//-----------------------------------------------------------------------------
void qUserManagementDialog::onUnlockUser()
{
  Q_D(qUserManagementDialog);

  int userId = d->getSelectedUserId();
  if (userId < 0)
    {
      QMessageBox::warning(this, QString::fromUtf8("提示"),
                           QString::fromUtf8("请先选择要解锁的用户"));
      return;
    }

  UserInfo currentUser = UserManager::instance().getCurrentUser();
  if (currentUser.role != QString::fromUtf8("admin") && currentUser.role != "admin")
    {
      QMessageBox::warning(this, QString::fromUtf8("权限不足"),
                           QString::fromUtf8("仅管理员可以解锁用户"));
      return;
    }

  UserInfo user = UserManager::instance().getUserById(userId);
  if (!user.isLocked)
    {
      QMessageBox::information(this, QString::fromUtf8("提示"),
                               QString::fromUtf8("该用户未被锁定"));
      return;
    }

  if (UserManager::instance().unlockUser(userId))
    {
      QMessageBox::information(this, QString::fromUtf8("成功"),
                               QString::fromUtf8("用户已解锁"));
      d->refreshTable();
    }
  else
    {
      QMessageBox::critical(this, QString::fromUtf8("失败"),
                            UserManager::instance().getLastError());
    }
}

//-----------------------------------------------------------------------------
void qUserManagementDialog::onResetPassword()
{
  Q_D(qUserManagementDialog);

  UserInfo user = d->getSelectedUser();
  if (user.id < 0)
    {
      QMessageBox::warning(this, QString::fromUtf8("提示"),
                           QString::fromUtf8("请先选择要重置密码的用户"));
      return;
    }

  UserInfo currentUser = UserManager::instance().getCurrentUser();
  if (currentUser.role != QString::fromUtf8("admin") && currentUser.role != "admin")
    {
      QMessageBox::warning(this, QString::fromUtf8("权限不足"),
                           QString::fromUtf8("仅管理员可以重置用户密码"));
      return;
    }

  QString newPassword = QInputDialog::getText(this, QString::fromUtf8("重置密码"),
    QString::fromUtf8("为用户 \"%1\" 设置新密码：").arg(user.username),
    QLineEdit::Password);
  if (newPassword.isEmpty())
    {
      return;
    }
  if (newPassword.length() < 6)
    {
      QMessageBox::warning(this, QString::fromUtf8("输入错误"),
                           QString::fromUtf8("密码长度不能少于6位"));
      return;
    }

  if (UserManager::instance().resetPassword(user.id, newPassword))
    {
      QMessageBox::information(this, QString::fromUtf8("成功"),
                               QString::fromUtf8("密码已重置"));
      d->refreshTable();
    }
  else
    {
      QMessageBox::critical(this, QString::fromUtf8("失败"),
                            UserManager::instance().getLastError());
    }
}

//-----------------------------------------------------------------------------
void qUserManagementDialog::onRefresh()
{
  Q_D(qUserManagementDialog);
  d->refreshTable();
}

//-----------------------------------------------------------------------------
void qUserManagementDialog::onUserSelectionChanged()
{
  Q_D(qUserManagementDialog);

  int userId = d->getSelectedUserId();
  bool hasSelection = (userId >= 0);

  d->EditButton->setEnabled(hasSelection);
  d->DeleteButton->setEnabled(hasSelection);
  d->ResetPasswordButton->setEnabled(hasSelection);

  if (hasSelection)
    {
      UserInfo user = UserManager::instance().getUserById(userId);
      UserInfo currentUser = UserManager::instance().getCurrentUser();
      bool isAdmin = (currentUser.role == QString::fromUtf8("admin") || currentUser.role == "admin");
      bool notSelf = (userId != currentUser.id);

      d->LockButton->setEnabled(isAdmin && notSelf && !user.isLocked);
      d->UnlockButton->setEnabled(isAdmin && notSelf && user.isLocked);
    }
  else
    {
      d->LockButton->setEnabled(false);
      d->UnlockButton->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
void qUserManagementDialog::onUserDoubleClicked(QTableWidgetItem* /*item*/)
{
  onEditUser();
}
