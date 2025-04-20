/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#ifndef __qUserManagementDialog_h
#define __qUserManagementDialog_h

#include "qRadianceAppExport.h"

#include <QDialog>
#include <QScopedPointer>

class qUserManagementDialogPrivate;
class QTableWidgetItem;

class Q_RADIANCE_APP_EXPORT qUserManagementDialog : public QDialog
{
  Q_OBJECT

public:
  explicit qUserManagementDialog(QWidget* parent = nullptr);
  virtual ~qUserManagementDialog();

  int exec() override;

protected slots:
  void onAddUser();
  void onEditUser();
  void onDeleteUser();
  void onLockUser();
  void onUnlockUser();
  void onResetPassword();
  void onRefresh();
  void onUserSelectionChanged();
  void onUserDoubleClicked(QTableWidgetItem* item);

protected:
  QScopedPointer<qUserManagementDialogPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qUserManagementDialog)
  Q_DISABLE_COPY(qUserManagementDialog)
};

#endif
