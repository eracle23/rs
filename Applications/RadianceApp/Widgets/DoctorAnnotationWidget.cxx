/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#include "DoctorAnnotationWidget.h"

// Qt includes
#include <QLineEdit>
#include <QVBoxLayout>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>

// Slicer includes
#include "qSlicerApplication.h"
#include "qSlicerCoreApplication.h"
#include "vtkSlicerApplicationLogic.h"
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLNode.h>
#include <vtkCallbackCommand.h>
#include <vtkCommand.h>
#include <vtkNew.h>

const char* DoctorAnnotationWidget::DoctorAnnotationAttributeName = "DoctorAnnotation";
const double DoctorAnnotationWidget::AnnotationFontSizePt = 10.5;  // 五号

namespace
{
void selectionNodeCallback(vtkObject*, unsigned long, void* clientData, void*)
{
  DoctorAnnotationWidget* w = static_cast<DoctorAnnotationWidget*>(clientData);
  if (w)
    {
    QMetaObject::invokeMethod(w, "updateFromCurrentVolume", Qt::QueuedConnection);
    }
}
}

//-----------------------------------------------------------------------------
DoctorAnnotationWidget::DoctorAnnotationWidget(QWidget* parent)
  : QWidget(parent)
  , m_annotationEdit(nullptr)
  , m_layout(nullptr)
  , m_updatingFromNode(false)
  , m_selectionObserver(nullptr)
{
  setupUi();
  observeSelectionNode();
}

//-----------------------------------------------------------------------------
DoctorAnnotationWidget::~DoctorAnnotationWidget()
{
  if (m_selectionObserver)
    {
    m_selectionObserver->Delete();
    m_selectionObserver = nullptr;
    }
}

//-----------------------------------------------------------------------------
void DoctorAnnotationWidget::setupUi()
{
  m_layout = new QVBoxLayout(this);
  m_layout->setContentsMargins(8, 8, 8, 8);
  m_layout->setSpacing(6);

  // 不再在控件内显示「医生批注」标题，由 Dock 窗口标题统一显示
  m_annotationEdit = new QLineEdit(this);
  m_annotationEdit->setObjectName("DoctorAnnotationEdit");
  m_annotationEdit->setMaxLength(MaxAnnotationLength);
  m_annotationEdit->setPlaceholderText(QString::fromUtf8("限 %1 字以内").arg(MaxAnnotationLength));
  m_annotationEdit->setClearButtonEnabled(true);
  m_annotationEdit->setMinimumHeight(52);

  // 思源黑体：优先 Source Han Sans SC，其次 思源黑体，再次系统默认
  QFont font = m_annotationEdit->font();
  if (QFontDatabase().hasFamily(QString::fromUtf8("Source Han Sans SC")))
    {
    font.setFamily(QString::fromUtf8("Source Han Sans SC"));
    }
  else if (QFontDatabase().hasFamily(QString::fromUtf8("思源黑体")))
    {
    font.setFamily(QString::fromUtf8("思源黑体"));
    }
  else if (QFontDatabase().hasFamily("Noto Sans CJK SC"))
    {
    font.setFamily("Noto Sans CJK SC");
    }
  font.setPointSizeF(AnnotationFontSizePt);
  m_annotationEdit->setFont(font);

  m_layout->addWidget(m_annotationEdit);

  connect(m_annotationEdit, &QLineEdit::textChanged, this, &DoctorAnnotationWidget::onAnnotationTextChanged);
}

//-----------------------------------------------------------------------------
void DoctorAnnotationWidget::observeSelectionNode()
{
  qSlicerCoreApplication* app = qSlicerCoreApplication::application();
  if (!app || !app->applicationLogic())
    {
    return;
    }
  vtkMRMLSelectionNode* selectionNode = app->applicationLogic()->GetSelectionNode();
  if (!selectionNode)
    {
    return;
    }
  m_selectionObserver = vtkCallbackCommand::New();
  m_selectionObserver->SetCallback(selectionNodeCallback);
  m_selectionObserver->SetClientData(this);
  selectionNode->AddObserver(vtkCommand::ModifiedEvent, m_selectionObserver);
  updateFromCurrentVolume();
}

//-----------------------------------------------------------------------------
void DoctorAnnotationWidget::onAnnotationTextChanged(const QString& text)
{
  if (m_updatingFromNode)
    {
    return;
    }
  qSlicerCoreApplication* app = qSlicerCoreApplication::application();
  if (!app || !app->applicationLogic())
    {
    return;
    }
  vtkMRMLSelectionNode* selectionNode = app->applicationLogic()->GetSelectionNode();
  if (!selectionNode)
    {
    return;
    }
  vtkMRMLScene* scene = app->mrmlScene();
  if (!scene)
    {
    return;
    }
  const char* volumeID = selectionNode->GetActiveVolumeID();
  if (!volumeID || !*volumeID)
    {
    return;
    }
  vtkMRMLNode* node = scene->GetNodeByID(volumeID);
  if (!node)
    {
    return;
    }
  QString toStore = text.left(MaxAnnotationLength);
  node->SetAttribute(DoctorAnnotationAttributeName, toStore.toUtf8().constData());
}

//-----------------------------------------------------------------------------
void DoctorAnnotationWidget::onSelectionNodeModified()
{
    updateFromCurrentVolume();
}

//-----------------------------------------------------------------------------
void DoctorAnnotationWidget::updateFromCurrentVolume()
{
  m_updatingFromNode = true;
  m_annotationEdit->clear();
  qSlicerCoreApplication* app = qSlicerCoreApplication::application();
  if (app && app->applicationLogic())
    {
    vtkMRMLSelectionNode* selectionNode = app->applicationLogic()->GetSelectionNode();
    vtkMRMLScene* scene = app->mrmlScene();
    if (selectionNode && scene)
      {
      const char* volumeID = selectionNode->GetActiveVolumeID();
      if (volumeID && *volumeID)
        {
        vtkMRMLNode* node = scene->GetNodeByID(volumeID);
        if (node)
          {
          const char* attr = node->GetAttribute(DoctorAnnotationAttributeName);
          if (attr && *attr)
            {
            m_annotationEdit->setText(QString::fromUtf8(attr));
            }
          }
        }
      }
    }
  m_updatingFromNode = false;
}
