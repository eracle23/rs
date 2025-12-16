/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#ifndef __UserManager_h
#define __UserManager_h

#include "qRadianceAppExport.h"

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVector>

struct UserInfo
{
  int id;
  QString username;
  QString passwordHash;
  QString role;
  QString fullName;
  QString email;
  QDateTime createdAt;
  QDateTime lastLogin;
  bool isActive;
  bool isLocked;
  int failedAttempts;
};

class Q_RADIANCE_APP_EXPORT UserManager : public QObject
{
  Q_OBJECT

public:
  static UserManager& instance();

  bool initialize();
  bool authenticate(const QString& username, const QString& password);
  bool addUser(const UserInfo& user);
  bool updateUser(const UserInfo& user);
  bool deleteUser(int userId);
  bool changePassword(int userId, const QString& oldPassword, const QString& newPassword);
  bool resetPassword(int userId, const QString& newPassword);
  QVector<UserInfo> getAllUsers();
  UserInfo getUserById(int userId);
  UserInfo getUserByUsername(const QString& username);
  bool lockUser(int userId);
  bool unlockUser(int userId);
  bool isUserLoggedIn();
  UserInfo getCurrentUser();
  void logout();

  QString getLastError();
  QString hashPassword(const QString& password);

signals:
  void userLoggedIn(const UserInfo& user);
  void userLoggedOut();
  void userAdded(const UserInfo& user);
  void userUpdated(const UserInfo& user);
  void userDeleted(int userId);
  void userLocked(int userId);
  void userUnlocked(int userId);

private:
  UserManager(QObject* parent = nullptr);
  virtual ~UserManager();

  bool createDefaultAdmin();

  Q_DISABLE_COPY(UserManager)

  class Impl;
  Impl* d;
};

#endif
