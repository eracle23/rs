/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#include "SliceColorAdapter.h"
#include "ThemeSync.h"
#include "../BrandingPreferences.h"

// Qt includes
#include <QColor>

// Slicer includes
#include "qSlicerApplication.h"

// VTK/MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLSliceNode.h>

#include <cmath>
#include <vector>

namespace
{

bool colorsNearlyEqual(const QColor& lhs, const QColor& rhs)
{
  constexpr double epsilon = 0.004;
  return (std::abs(lhs.redF() - rhs.redF()) < epsilon) &&
         (std::abs(lhs.greenF() - rhs.greenF()) < epsilon) &&
         (std::abs(lhs.blueF() - rhs.blueF()) < epsilon);
}

/// 将切片颜色调整到品牌主题适合的饱和度/明度范围
QColor tunedSliceColor(const QColor& source)
{
  QColor hsv = source.toHsv();
  if (!hsv.isValid())
    {
    return source;
    }
  int hue   = hsv.hue();
  int sat   = hsv.saturation();
  int val   = hsv.value();
  int alpha = hsv.alpha();

  constexpr int minSaturation = 140;
  constexpr int maxSaturation = 235;
  constexpr int minValue      = 180;
  constexpr int maxValue      = 240;
  constexpr int minAlpha      = 220;

  sat   = qBound(minSaturation, sat, maxSaturation);
  val   = qBound(minValue,      val, maxValue);
  alpha = qBound(minAlpha,      alpha, 255);

  QColor tuned;
  tuned.setHsv(hue, sat, val, alpha);
  return tuned;
}

} // namespace

//-----------------------------------------------------------------------------
SliceColorAdapter::SliceColorAdapter(ThemeSync* themeSync, QObject* parent)
  : QObject(parent)
{
  if (themeSync)
    {
    QObject::connect(themeSync, &ThemeSync::brandingApplied,
                     this,      &SliceColorAdapter::onBrandingApplied);
    }
}

//-----------------------------------------------------------------------------
void SliceColorAdapter::onBrandingApplied()
{
  if (!RadianceBranding::themeSwitchAllowed())
    {
    return;
    }

  qSlicerApplication* app = qSlicerApplication::application();
  if (!app)
    {
    return;
    }

  vtkMRMLScene* scene = app->mrmlScene();
  if (!scene)
    {
    return;
    }

  std::vector<vtkMRMLNode*> sliceNodes;
  scene->GetNodesByClass("vtkMRMLSliceNode", sliceNodes);

  for (vtkMRMLNode* node : sliceNodes)
    {
    auto* sliceNode = vtkMRMLSliceNode::SafeDownCast(node);
    if (!sliceNode)
      {
      continue;
      }

    const char* id = sliceNode->GetID();
    if (!id || id[0] == '\0')
      {
      continue;
      }

    const QString nodeId = QString::fromUtf8(id);

    // 跳过用户手动修改过颜色的节点
    if (this->userOverriddenSliceNodes_.contains(nodeId))
      {
      continue;
      }

    double rgb[3] = {0., 0., 0.};
    sliceNode->GetLayoutColor(rgb);
    const QColor current = QColor::fromRgbF(rgb[0], rgb[1], rgb[2]);

    // 如果已记录过调整后的颜色，检查用户是否又手动改了
    const auto storedIt = this->tunedSliceNodeColors_.find(nodeId);
    if (storedIt != this->tunedSliceNodeColors_.end())
      {
      if (!colorsNearlyEqual(current, storedIt.value()))
        {
        // 用户改过了，停止自动调整
        this->userOverriddenSliceNodes_.insert(nodeId);
        this->tunedSliceNodeColors_.remove(nodeId);
        }
      continue;
      }

    const QColor tuned = tunedSliceColor(current);
    if (colorsNearlyEqual(tuned, current))
      {
      continue;
      }

    const double newRgb[3] = { tuned.redF(), tuned.greenF(), tuned.blueF() };
    sliceNode->SetLayoutColor(newRgb);
    this->tunedSliceNodeColors_.insert(nodeId, tuned);
    }
}
