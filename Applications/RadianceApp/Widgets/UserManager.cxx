/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#include "UserManager.h"

// Qt includes
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

class UserManager::Impl
{
public:
  QSqlDatabase database;
  UserInfo currentUser;
  bool isLoggedIn;
  QString lastError;
};

//-----------------------------------------------------------------------------
UserManager& UserManager::instance()
{
  static UserManager instance;
  return instance;
}

//-----------------------------------------------------------------------------
UserManager::UserManager(QObject* parent)
  : QObject(parent)
  , d(new Impl)
{
  d->isLoggedIn = false;
}

//-----------------------------------------------------------------------------
UserManager::~UserManager()
{
  if (d->database.isOpen())
    {
      d->database.close();
    }
  delete d;
}

//-----------------------------------------------------------------------------
bool UserManager::initialize()
{
  QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  QDir().mkpath(dbPath);
  dbPath += "/users.db";

  d->database = QSqlDatabase::addDatabase("QSQLITE", "RadianceUsers");
  d->database.setDatabaseName(dbPath);

  if (!d->database.open())
    {
      d->lastError = QString::fromUtf8("无法打开用户数据库: %1")
        .arg(d->database.lastError().text());
      qDebug() << d->lastError;
      return false;
    }

  QSqlQuery query(d->database);

  bool success = true;

  success = success && query.exec(
    "CREATE TABLE IF NOT EXISTS users ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "username TEXT UNIQUE NOT NULL, "
    "password_hash TEXT NOT NULL, "
    "role TEXT NOT NULL DEFAULT 'user', "
    "full_name TEXT, "
    "email TEXT, "
    "created_at TEXT NOT NULL, "
    "last_login TEXT, "
    "is_active INTEGER NOT NULL DEFAULT 1, "
    "is_locked INTEGER NOT NULL DEFAULT 0, "
    "failed_attempts INTEGER NOT NULL DEFAULT 0"
    ")"
  );

  if (!success)
    {
      d->lastError = QString::fromUtf8("创建用户表失败: %1")
        .arg(query.lastError().text());
      qDebug() << d->lastError;
      return false;
    }

  success = createDefaultAdmin();

  return success;
}

//-----------------------------------------------------------------------------
bool UserManager::createDefaultAdmin()
{
  QSqlQuery query(d->database);

  query.prepare("SELECT COUNT(*) FROM users WHERE username = 'admin'");
  if (!query.exec())
    {
      d->lastError = QString::fromUtf8("查询管理员账户失败: %1")
        .arg(query.lastError().text());
      qDebug() << d->lastError;
      return false;
    }

  if (query.next() && query.value(0).toInt() > 0)
    {
      return true;
    }

  QString defaultPassword = QString::fromUtf8("admin123");
  QString passwordHash = hashPassword(defaultPassword);

  query.prepare(
    "INSERT INTO users (username, password_hash, role, full_name, email, created_at, is_active, is_locked) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
  );

  query.addBindValue("admin");
  query.addBindValue(passwordHash);
  query.addBindValue("admin");
  query.addBindValue(QString::fromUtf8("系统管理员"));
  query.addBindValue("admin@radiancelabs.com");
  query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
  query.addBindValue(1);
  query.addBindValue(0);

  if (!query.exec())
    {
      d->lastError = QString::fromUtf8("创建默认管理员账户失败: %1")
        .arg(query.lastError().text());
      qDebug() << d->lastError;
      return false;
    }

  qDebug() << "默认管理员账户已创建";
  qDebug() << "用户名: admin";
  qDebug() << "密码: admin123";
  qDebug() << "请在首次登录后立即修改密码！";

  return true;
}

//-----------------------------------------------------------------------------
QString UserManager::hashPassword(const QString& password)
{
  QByteArray hash = QCryptographicHash::hash(
    password.toUtf8(),
    QCryptographicHash::Sha256
  );
  return hash.toHex();
}

//-----------------------------------------------------------------------------
bool UserManager::authenticate(const QString& username, const QString& password)
{
  QSqlQuery query(d->database);

  query.prepare(
    "SELECT id, username, password_hash, role, full_name, email, "
    "created_at, last_login, is_active, is_locked, failed_attempts "
    "FROM users WHERE username = ?"
  );
  query.addBindValue(username);

  if (!query.exec())
    {
      d->lastError = QString::fromUtf8("查询用户失败: %1")
        .arg(query.lastError().text());
      qDebug() << d->lastError;
      return false;
    }

  if (!query.next())
    {
      d->lastError = QString::fromUtf8("用户名或密码错误");
      return false;
    }

  UserInfo user;
  user.id = query.value(0).toInt();
  user.username = query.value(1).toString();
  user.passwordHash = query.value(2).toString();
  user.role = query.value(3).toString();
  user.fullName = query.value(4).toString();
  user.email = query.value(5).toString();
  user.createdAt = QDateTime::fromString(query.value(6).toString(), Qt::ISODate);
  user.lastLogin = QDateTime::fromString(query.value(7).toString(), Qt::ISODate);
  user.isActive = query.value(8).toBool();
  user.isLocked = query.value(9).toBool();
  user.failedAttempts = query.value(10).toInt();

  if (!user.isActive)
    {
      d->lastError = QString::fromUtf8("该账户已被禁用");
      return false;
    }

  if (user.isLocked)
    {
      d->lastError = QString::fromUtf8("该账户已被锁定，请联系管理员");
      return false;
    }

  QString passwordHash = hashPassword(password);
  if (user.passwordHash != passwordHash)
    {
      user.failedAttempts++;

      QSqlQuery updateQuery(d->database);
      updateQuery.prepare(
        "UPDATE users SET failed_attempts = ? WHERE id = ?"
      );
      updateQuery.addBindValue(user.failedAttempts);
      updateQuery.addBindValue(user.id);
      updateQuery.exec();

      if (user.failedAttempts >= 5)
        {
          QSqlQuery lockQuery(d->database);
          lockQuery.prepare("UPDATE users SET is_locked = 1 WHERE id = ?");
          lockQuery.addBindValue(user.id);
          lockQuery.exec();

          emit userLocked(user.id);

          d->lastError = QString::fromUtf8("密码错误次数过多，账户已被锁定");
        }
      else
        {
          d->lastError = QString::fromUtf8("用户名或密码错误（剩余尝试次数: %1）")
            .arg(5 - user.failedAttempts);
        }

      return false;
    }

  QSqlQuery updateQuery(d->database);
  updateQuery.prepare(
    "UPDATE users SET last_login = ?, failed_attempts = 0 WHERE id = ?"
  );
  updateQuery.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
  updateQuery.addBindValue(user.id);
  updateQuery.exec();

  d->currentUser = user;
  d->isLoggedIn = true;

  emit userLoggedIn(user);

  return true;
}

//-----------------------------------------------------------------------------
bool UserManager::addUser(const UserInfo& user)
{
  QSqlQuery query(d->database);

  query.prepare(
    "INSERT INTO users (username, password_hash, role, full_name, email, created_at, is_active, is_locked) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
  );

  query.addBindValue(user.username);
  query.addBindValue(user.passwordHash);
  query.addBindValue(user.role);
  query.addBindValue(user.fullName);
  query.addBindValue(user.email);
  query.addBindValue(user.createdAt.toString(Qt::ISODate));
  query.addBindValue(user.isActive ? 1 : 0);
  query.addBindValue(user.isLocked ? 1 : 0);

  if (!query.exec())
    {
      d->lastError = QString::fromUtf8("添加用户失败: %1")
        .arg(query.lastError().text());
      qDebug() << d->lastError;
      return false;
    }

  emit userAdded(user);
  return true;
}

//-----------------------------------------------------------------------------
bool UserManager::updateUser(const UserInfo& user)
{
  QSqlQuery query(d->database);

  query.prepare(
    "UPDATE users SET username = ?, role = ?, full_name = ?, email = ?, is_active = ? WHERE id = ?"
  );

  query.addBindValue(user.username);
  query.addBindValue(user.role);
  query.addBindValue(user.fullName);
  query.addBindValue(user.email);
  query.addBindValue(user.isActive ? 1 : 0);
  query.addBindValue(user.id);

  if (!query.exec())
    {
      d->lastError = QString::fromUtf8("更新用户失败: %1")
        .arg(query.lastError().text());
      qDebug() << d->lastError;
      return false;
    }

  emit userUpdated(user);
  return true;
}

//-----------------------------------------------------------------------------
bool UserManager::deleteUser(int userId)
{
  QSqlQuery query(d->database);

  query.prepare("DELETE FROM users WHERE id = ?");
  query.addBindValue(userId);

  if (!query.exec())
    {
      d->lastError = QString::fromUtf8("删除用户失败: %1")
        .arg(query.lastError().text());
      qDebug() << d->lastError;
      return false;
    }

  emit userDeleted(userId);
  return true;
}

//-----------------------------------------------------------------------------
bool UserManager::changePassword(int userId, const QString& oldPassword, const QString& newPassword)
{
  QSqlQuery query(d->database);

  query.prepare("SELECT password_hash FROM users WHERE id = ?");
  query.addBindValue(userId);

  if (!query.exec() || !query.next())
    {
      d->lastError = QString::fromUtf8("查询用户失败");
      return false;
    }

  QString currentHash = query.value(0).toString();
  QString oldHash = hashPassword(oldPassword);

  if (currentHash != oldHash)
    {
      d->lastError = QString::fromUtf8("原密码错误");
      return false;
    }

  QString newHash = hashPassword(newPassword);

  QSqlQuery updateQuery(d->database);
  updateQuery.prepare("UPDATE users SET password_hash = ? WHERE id = ?");
  updateQuery.addBindValue(newHash);
  updateQuery.addBindValue(userId);

  if (!updateQuery.exec())
    {
      d->lastError = QString::fromUtf8("修改密码失败: %1")
        .arg(updateQuery.lastError().text());
      qDebug() << d->lastError;
      return false;
    }

  return true;
}

//-----------------------------------------------------------------------------
bool UserManager::resetPassword(int userId, const QString& newPassword)
{
  QString newHash = hashPassword(newPassword);

  QSqlQuery query(d->database);
  query.prepare("UPDATE users SET password_hash = ?, failed_attempts = 0, is_locked = 0 WHERE id = ?");
  query.addBindValue(newHash);
  query.addBindValue(userId);

  if (!query.exec())
    {
      d->lastError = QString::fromUtf8("重置密码失败: %1")
        .arg(query.lastError().text());
      qDebug() << d->lastError;
      return false;
    }

  emit userUnlocked(userId);
  return true;
}

//-----------------------------------------------------------------------------
QVector<UserInfo> UserManager::getAllUsers()
{
  QVector<UserInfo> users;
  QSqlQuery query(d->database);

  if (!query.exec("SELECT id, username, role, full_name, email, created_at, last_login, is_active, is_locked FROM users ORDER BY id"))
    {
      d->lastError = QString::fromUtf8("查询用户列表失败: %1")
        .arg(query.lastError().text());
      qDebug() << d->lastError;
      return users;
    }

  while (query.next())
    {
      UserInfo user;
      user.id = query.value(0).toInt();
      user.username = query.value(1).toString();
      user.role = query.value(2).toString();
      user.fullName = query.value(3).toString();
      user.email = query.value(4).toString();
      user.createdAt = QDateTime::fromString(query.value(5).toString(), Qt::ISODate);
      user.lastLogin = QDateTime::fromString(query.value(6).toString(), Qt::ISODate);
      user.isActive = query.value(7).toBool();
      user.isLocked = query.value(8).toBool();
      users.append(user);
    }

  return users;
}

//-----------------------------------------------------------------------------
UserInfo UserManager::getUserById(int userId)
{
  UserInfo user;
  QSqlQuery query(d->database);

  query.prepare("SELECT id, username, role, full_name, email, created_at, last_login, is_active, is_locked FROM users WHERE id = ?");
  query.addBindValue(userId);

  if (!query.exec() || !query.next())
    {
      d->lastError = QString::fromUtf8("查询用户失败");
      return user;
    }

  user.id = query.value(0).toInt();
  user.username = query.value(1).toString();
  user.role = query.value(2).toString();
  user.fullName = query.value(3).toString();
  user.email = query.value(4).toString();
  user.createdAt = QDateTime::fromString(query.value(5).toString(), Qt::ISODate);
  user.lastLogin = QDateTime::fromString(query.value(6).toString(), Qt::ISODate);
  user.isActive = query.value(7).toBool();
  user.isLocked = query.value(8).toBool();

  return user;
}

//-----------------------------------------------------------------------------
UserInfo UserManager::getUserByUsername(const QString& username)
{
  UserInfo user;
  QSqlQuery query(d->database);

  query.prepare("SELECT id, username, role, full_name, email, created_at, last_login, is_active, is_locked FROM users WHERE username = ?");
  query.addBindValue(username);

  if (!query.exec() || !query.next())
    {
      d->lastError = QString::fromUtf8("查询用户失败");
      return user;
    }

  user.id = query.value(0).toInt();
  user.username = query.value(1).toString();
  user.role = query.value(2).toString();
  user.fullName = query.value(3).toString();
  user.email = query.value(4).toString();
  user.createdAt = QDateTime::fromString(query.value(5).toString(), Qt::ISODate);
  user.lastLogin = QDateTime::fromString(query.value(6).toString(), Qt::ISODate);
  user.isActive = query.value(7).toBool();
  user.isLocked = query.value(8).toBool();

  return user;
}

//-----------------------------------------------------------------------------
bool UserManager::lockUser(int userId)
{
  QSqlQuery query(d->database);
  query.prepare("UPDATE users SET is_locked = 1 WHERE id = ?");
  query.addBindValue(userId);

  if (!query.exec())
    {
      d->lastError = QString::fromUtf8("锁定用户失败: %1")
        .arg(query.lastError().text());
      qDebug() << d->lastError;
      return false;
    }

  emit userLocked(userId);
  return true;
}

//-----------------------------------------------------------------------------
bool UserManager::unlockUser(int userId)
{
  QSqlQuery query(d->database);
  query.prepare("UPDATE users SET is_locked = 0, failed_attempts = 0 WHERE id = ?");
  query.addBindValue(userId);

  if (!query.exec())
    {
      d->lastError = QString::fromUtf8("解锁用户失败: %1")
        .arg(query.lastError().text());
      qDebug() << d->lastError;
      return false;
    }

  emit userUnlocked(userId);
  return true;
}

//-----------------------------------------------------------------------------
bool UserManager::isUserLoggedIn()
{
  return d->isLoggedIn;
}

//-----------------------------------------------------------------------------
UserInfo UserManager::getCurrentUser()
{
  return d->currentUser;
}

//-----------------------------------------------------------------------------
void UserManager::logout()
{
  d->currentUser = UserInfo();
  d->isLoggedIn = false;

  emit userLoggedOut();
}

//-----------------------------------------------------------------------------
QString UserManager::getLastError()
{
  return d->lastError;
}
