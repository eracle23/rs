/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#ifndef __VolumeImportValidator_h
#define __VolumeImportValidator_h

// Radiance includes
#include "qRadianceAppExport.h"

#include <QString>

class vtkMRMLNode;
class vtkMRMLScene;

/**
 * @brief 影像导入校验
 *
 * 限制：仅 DICOM、直尺长度 ≤ 2.5 m、角度 ≤ 180°、阈值 -1050～2500。
 * 校验不通过返回错误说明，通过返回空字符串。
 */
class Q_RADIANCE_APP_EXPORT VolumeImportValidator
{
public:
  /// 直尺长度上限（mm），2.5 米
  static const double MaxRulerLengthMm;

  /// 角度上限（度），180°
  static const double MaxAngleDegrees;

  /// 阈值下限
  static const double ThresholdMin;

  /// 阈值上限
  static const double ThresholdMax;

  /**
   * @brief 校验 Volume 节点是否符合导入要求
   * @param node 待校验的 MRML 节点（应为 vtkMRMLScalarVolumeNode）
   * @return 空字符串表示通过；非空为错误提示（格式错误/尺寸超限/角度超限/阈值超限）
   */
  static QString validate(vtkMRMLNode* node);

  /**
   * @brief 向场景注册 NodeAdded 观察者，对每个新加入的 Volume 节点自动执行校验。
   *
   * 校验不通过时会通过 Qt 主线程弹出警告对话框并从场景中移除该节点。
   * 若 scene 为 nullptr 则不执行任何操作。
   * 观察者的生命周期与 scene 绑定，无需手动释放。
   *
   * @param scene 要监听的 MRML 场景
   */
  static void installSceneObserver(vtkMRMLScene* scene);
};

#endif
