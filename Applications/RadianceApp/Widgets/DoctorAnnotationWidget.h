/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#ifndef __DoctorAnnotationWidget_h
#define __DoctorAnnotationWidget_h

// Radiance includes
#include "qRadianceAppExport.h"

#include <QWidget>

// VTK forward
class vtkCallbackCommand;

class QLineEdit;
class QVBoxLayout;

/**
 * @brief 医生批注控件
 *
 * 提供一栏输入批注信息，限 50 字，使用思源黑体显示。
 * 批注随当前选中的图像（MRML Volume 节点）一并保存到场景中（节点属性 DoctorAnnotation）。
 */
class Q_RADIANCE_APP_EXPORT DoctorAnnotationWidget : public QWidget
{
  Q_OBJECT

public:
  explicit DoctorAnnotationWidget(QWidget* parent = nullptr);
  ~DoctorAnnotationWidget() override;

  /// 批注最大字数
  static const int MaxAnnotationLength = 50;

  /// 批注文字字号（pt）。五号 ≈ 10.5 pt；小五 ≈ 9 pt；四号 ≈ 12 pt
  static const double AnnotationFontSizePt;

  /// MRML 节点上存储批注的属性名
  static const char* DoctorAnnotationAttributeName;

protected slots:
  void onAnnotationTextChanged(const QString& text);
  void onSelectionNodeModified();
  void updateFromCurrentVolume();

protected:
  void setupUi();
  void observeSelectionNode();

  QLineEdit* m_annotationEdit;
  QVBoxLayout* m_layout;
  bool m_updatingFromNode;
  vtkCallbackCommand* m_selectionObserver;
};

#endif
