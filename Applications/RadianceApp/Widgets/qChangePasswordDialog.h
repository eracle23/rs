/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#ifndef __qChangePasswordDialog_h
#define __qChangePasswordDialog_h

#include "qRadianceAppExport.h"

#include <QDialog>
#include <QScopedPointer>

class qChangePasswordDialogPrivate;

class Q_RADIANCE_APP_EXPORT qChangePasswordDialog : public QDialog
{
  Q_OBJECT

public:
  explicit qChangePasswordDialog(QWidget* parent = nullptr);
  virtual ~qChangePasswordDialog();

  int exec() override;

protected slots:
  void onChangePassword();

protected:
  QScopedPointer<qChangePasswordDialogPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qChangePasswordDialog)
  Q_DISABLE_COPY(qChangePasswordDialog)
};

#endif
