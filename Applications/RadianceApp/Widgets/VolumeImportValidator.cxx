/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#include "VolumeImportValidator.h"

#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLStorageNode.h>
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkMath.h>
#include <vtkNew.h>

#include <QFileInfo>
#include <cmath>
#include <cstring>

const double VolumeImportValidator::MaxRulerLengthMm = 2500.0;   // 2.5 m
const double VolumeImportValidator::MaxAngleDegrees = 180.0;
const double VolumeImportValidator::ThresholdMin = -1050.0;
const double VolumeImportValidator::ThresholdMax = 2500.0;

namespace
{

/// 判断是否为 DICOM 来源：扩展名为 DICOM 常见后缀，或路径来自 DICOM 缓存（Slicer 从 DICOM 模块加载后可能存为 .nrrd/.nii）
bool isDicomSource(const QString& path)
{
  if (path.isEmpty())
    return false;
  QFileInfo fi(path);
  QString ext = fi.suffix().toLower();
  QString pathLower = path.toLower();
  // 常见 DICOM 扩展名或无扩展名
  if (ext == "dcm" || ext == "dicom" || ext == "dc3" || ext == "dic" || ext.isEmpty())
    return true;
  // 从 DICOM 模块加载时 Slicer 可能将缓存存为 nrrd/nii，路径中通常含 DICOM
  if (pathLower.contains("dicom") && (ext == "nrrd" || ext == "nii" || (ext == "gz" && pathLower.endsWith(".nii.gz"))))
    return true;
  return false;
}

}

//-----------------------------------------------------------------------------
QString VolumeImportValidator::validate(vtkMRMLNode* node)
{
  if (!node)
    {
    return QString::fromUtf8("【无效】节点无效，无法校验。");
    }
  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  if (!volumeNode)
    {
    return QString();  // 非 Volume 不校验
    }

  // 仅对“从文件导入”的 Volume 做导入校验（有存储节点）；无存储节点则跳过
  vtkMRMLStorageNode* storageNode = volumeNode->GetStorageNode();
  if (!storageNode)
    {
    return QString();
    }
  // 1. 格式：仅允许 DICOM（含从 DICOM 模块加载后缓存为 nrrd/nii 的情况）
  const char* attrDicom = volumeNode->GetAttribute("LoadedFromDICOM");
  bool markedFromDicom = (attrDicom && (strcmp(attrDicom, "1") == 0 || strcmp(attrDicom, "true") == 0));
  if (!markedFromDicom)
    {
    const char* fileName = storageNode->GetFileName();
    if (fileName && *fileName)
      {
      if (!isDicomSource(QString::fromUtf8(fileName)))
        {
        return QString::fromUtf8("【格式错误】仅支持 DICOM 影像，当前文件格式不符合要求。请通过 DICOM 模块或 DICOM 格式文件导入。");
        }
      }
    }

  vtkImageData* imageData = volumeNode->GetImageData();
  if (!imageData)
    {
    return QString();  // 尚无图像数据，跳过后续
    }

  // 2. 尺寸：直尺长度 ≤ 2.5 m（任一边物理长度不超过 2500 mm）
  double spacing[3];
  volumeNode->GetSpacing(spacing);
  int dims[3];
  imageData->GetDimensions(dims);
  for (int i = 0; i < 3; ++i)
    {
    double lengthMm = std::abs(static_cast<double>(dims[i]) * spacing[i]);
    if (lengthMm > MaxRulerLengthMm)
      {
      return QString::fromUtf8("【尺寸超限】影像任一边物理长度不得超过 2.5 米，当前约 %1 米。")
        .arg(lengthMm / 1000.0, 0, 'f', 2);
      }
    }

  // 3. 角度：切片法线与 Z 轴夹角 ≤ 180°（几何上恒在 [0,180]；> 90° 视为过度倾斜，提示角度超限）
 // vtkMatrix4x4* ijkToRAS = volumeNode->GetIJKToRASMatrix();
  vtkNew<vtkMatrix4x4> ijkToRAS; // 在栈上创建智能指针，管理堆上的矩阵
  volumeNode->GetIJKToRASMatrix(ijkToRAS); // 传入指针
  if (ijkToRAS)
    {
    double normal[3] = {
      ijkToRAS->GetElement(0, 2),
      ijkToRAS->GetElement(1, 2),
      ijkToRAS->GetElement(2, 2)
    };
    double z[3] = { 0, 0, 1 };
    double dot = vtkMath::Dot(normal, z);
    double angleDeg = std::acos(std::max(-1.0, std::min(1.0, dot))) * 180.0 / vtkMath::Pi();
    if (angleDeg > MaxAngleDegrees)
      {
      return QString::fromUtf8("【角度超限】切片角度不得超过 180°，当前约 %1°。").arg(angleDeg, 0, 'f', 1);
      }
    }

  // 4. 阈值：标量范围在 -1050 ～ 2500 内
  double range[2];
  imageData->GetScalarRange(range);
  if (range[0] < ThresholdMin)
    {
    return QString::fromUtf8("【阈值超限】影像灰度范围须在 -1050 至 2500 之间，当前最小值为 %1。").arg(range[0], 0, 'f', 0);
    }
  if (range[1] > ThresholdMax)
    {
    return QString::fromUtf8("【阈值超限】影像灰度范围须在 -1050 至 2500 之间，当前最大值为 %1。").arg(range[1], 0, 'f', 0);
    }

  return QString();
}
