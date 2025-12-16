/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#ifndef __qLoginDialog_h
#define __qLoginDialog_h

#include "qRadianceAppExport.h"

#include <QDialog>
#include <QPoint>
#include <QScopedPointer>

class qLoginDialogPrivate;

class Q_RADIANCE_APP_EXPORT qLoginDialog : public QDialog
{
  Q_OBJECT

public:
  explicit qLoginDialog(QWidget* parent = nullptr);
  virtual ~qLoginDialog();

  int exec() override;

protected:
  bool eventFilter(QObject* obj, QEvent* event) override;

protected slots:
  void onLoginClicked();
  void onUsernameChanged(const QString& text);
  void onPasswordChanged(const QString& text);
  void onForgotPassword();

protected:
  QScopedPointer<qLoginDialogPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qLoginDialog)
  Q_DISABLE_COPY(qLoginDialog)
};

#endif
