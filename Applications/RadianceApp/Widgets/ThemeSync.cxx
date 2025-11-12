// Minimal shell-level theme synchronizer implementation.

#include "ThemeSync.h"
#include "../BrandingPreferences.h"

#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QPalette>
#include <QStyle>
#include <QTimer>
#include <QWidget>
#include <QString>

#include <cmath>

#include "qSlicerApplication.h"
#include <vtkMRMLScene.h>
#include <vtkMRMLSliceNode.h>
#include <vector>

namespace
{

QString rgbaString(const QColor& color)
{
  return QStringLiteral("rgba(%1,%2,%3,%4)")
    .arg(color.red())
    .arg(color.green())
    .arg(color.blue())
    .arg(QString::number(color.alphaF(), 'f', 3));
}

bool colorsNearlyEqual(const QColor& lhs, const QColor& rhs)
{
  constexpr double epsilon = 0.004;
  return (std::abs(lhs.redF() - rhs.redF()) < epsilon) &&
         (std::abs(lhs.greenF() - rhs.greenF()) < epsilon) &&
         (std::abs(lhs.blueF() - rhs.blueF()) < epsilon);
}

QColor tunedSliceColor(const QColor& source)
{
  QColor hsv = source.toHsv();
  if (!hsv.isValid())
    {
    return source;
    }
  int hue = hsv.hue();
  int sat = hsv.saturation();
  int val = hsv.value();
  int alpha = hsv.alpha();

  constexpr int minSaturation = 140;
  constexpr int maxSaturation = 235;
  constexpr int minValue = 180;
  constexpr int maxValue = 240;
  constexpr int minAlpha = 220;

  if (sat > maxSaturation)
    {
    sat = maxSaturation;
    }
  else if (sat < minSaturation)
    {
    sat = minSaturation;
    }

  if (val < minValue)
    {
    val = minValue;
    }
  else if (val > maxValue)
    {
    val = maxValue;
    }

  if (alpha < minAlpha)
    {
    alpha = minAlpha;
    }
  else if (alpha > 255)
    {
    alpha = 255;
    }

  QColor tuned;
  tuned.setHsv(hue, sat, val, alpha);
  return tuned;
}

} // namespace

ThemeSync::ThemeSync(QObject* parent)
  : QObject(parent)
{
  // 环境变量紧急开关：YOURAPP_DISABLE_THEMESYNC=1 可完全禁用主题同步
  if (qEnvironmentVariableIntValue("YOURAPP_DISABLE_THEMESYNC") == 1)
    {
    return;
    }

  if (qApp)
    {
    qApp->installEventFilter(this);
    }
  // 启动后首次延迟应用（真实创建在 startupCompleted 之后，见主窗口）
  QTimer::singleShot(0, this, &ThemeSync::applyBranding);
}

bool ThemeSync::eventFilter(QObject* /*watched*/, QEvent* event)
{
  switch (event->type())
    {
    case QEvent::ApplicationPaletteChange:
    case QEvent::PaletteChange:
    case QEvent::StyleChange:
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
    case QEvent::ThemeChange:
#endif
      if (!this->pending_)
        {
        this->pending_ = true;
        QTimer::singleShot(50, this, &ThemeSync::applyBranding);
        }
      break;
    default:
      break;
    }
  return false;
}

void ThemeSync::applyBranding()
{
  if (this->applying_)
    {
    return; // 防止重入
    }

  this->applying_ = true;
  this->pending_ = false;

  // 再次检查紧急开关，运行期也可关闭
  if (qEnvironmentVariableIntValue("YOURAPP_DISABLE_THEMESYNC") == 1)
    {
    this->applying_ = false;
    return;
    }

  // 载入遵循 palette 角色的 QSS
  QString qss;
  {
    QFile f(":/Brand/brand.qss");
    if (f.open(QIODevice::ReadOnly))
      {
      qss = QString::fromUtf8(f.readAll());
      }
  }

  if (qApp)
    {
    // 仅在内容有变化时设置，避免触发无谓的 StyleChange 风暴
    QPalette currentPalette = qApp->palette();
    QPalette updatedPalette = currentPalette;

    QColor windowColor = currentPalette.color(QPalette::Window);
    if (!windowColor.isValid())
      {
      windowColor = currentPalette.color(QPalette::Base);
      }
    if (!windowColor.isValid())
      {
      windowColor = QColor(32, 32, 32);
      }
    const bool isDark = windowColor.lightness() < 140;

    QColor fallbackText = currentPalette.color(QPalette::WindowText);
    if (!fallbackText.isValid())
      {
      fallbackText = isDark ? QColor(245, 247, 250) : QColor(24, 24, 27);
      }

    QColor disabledText = fallbackText;
    disabledText = isDark ? disabledText.lighter(165) : disabledText.darker(135);

    updatedPalette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
    updatedPalette.setColor(QPalette::Disabled, QPalette::Text, disabledText);
    updatedPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
    updatedPalette.setColor(QPalette::Disabled, QPalette::PlaceholderText, disabledText);
    updatedPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledText);

    QString mergedQss = qss;
    const QString disabledQss = QStringLiteral(R"(
QWidget:disabled,
QLabel:disabled,
QToolButton:disabled,
QPushButton:disabled,
QCheckBox:disabled,
QRadioButton:disabled,
QGroupBox::title:disabled,
QComboBox:disabled,
QLineEdit:disabled,
QSpinBox:disabled,
QDoubleSpinBox:disabled,
QAbstractItemView:disabled {
  color: %1;
}
)").arg(rgbaString(disabledText));
    if (!disabledQss.isEmpty())
      {
      if (!mergedQss.isEmpty() && !mergedQss.endsWith(QLatin1Char('\n')))
        {
        mergedQss.append(QLatin1Char('\n'));
        }
      mergedQss.append(disabledQss);
      }

    if (qApp->styleSheet() != mergedQss)
      {
      qApp->setStyleSheet(mergedQss);
      }

    QColor tooltipBase = currentPalette.color(QPalette::Base);
    if (!tooltipBase.isValid())
      {
      tooltipBase = windowColor;
      }
    if (tooltipBase.lightness() < 128)
      {
      tooltipBase = tooltipBase.lighter(185);
      }
    else
      {
      tooltipBase = tooltipBase.darker(110);
      }
    QColor tooltipText = fallbackText;
    if (!tooltipText.isValid())
      {
      tooltipText = isDark ? QColor(248, 249, 252) : QColor(33, 37, 41);
      }
    updatedPalette.setColor(QPalette::ToolTipBase, tooltipBase);
    updatedPalette.setColor(QPalette::ToolTipText, tooltipText);

    if (updatedPalette != currentPalette)
      {
      qApp->setPalette(updatedPalette);
      }

    // 轻量刷新：仅顶层窗口 re-polish，避免 allWidgets 的高开销
    const auto topLevels = qApp->topLevelWidgets();
    for (auto* w : topLevels)
      {
      if (!w || !w->style())
        {
        continue;
        }
      w->style()->unpolish(w);
      w->style()->polish(w);
      w->update();
      }
    }

  this->refreshSliceControllers();

  this->applying_ = false;
}

void ThemeSync::refreshSliceControllers()
{
  if (RadianceBranding::nativeStyleEnabled())
    {
    return;
    }

  if (qSlicerApplication* app = qSlicerApplication::application())
    {
    if (vtkMRMLScene* scene = app->mrmlScene())
      {
      std::vector<vtkMRMLNode*> sliceNodes;
      scene->GetNodesByClass("vtkMRMLSliceNode", sliceNodes);
      for (vtkMRMLNode* node : sliceNodes)
        {
        auto* sliceNode = vtkMRMLSliceNode::SafeDownCast(node);
        if (!sliceNode)
          {
          continue;
          }
          {
          double rgb[3] = {0., 0., 0.};
          sliceNode->GetLayoutColor(rgb);

          const char* id = sliceNode->GetID();
          if (!id || id[0] == '\0')
            {
            continue;
            }

          const QString nodeId = QString::fromUtf8(id);
          if (this->userOverriddenSliceNodes_.contains(nodeId))
            {
            continue;
            }

          const QColor current = QColor::fromRgbF(rgb[0], rgb[1], rgb[2]);
          const auto storedIt = this->tunedSliceNodeColors_.find(nodeId);
          if (storedIt != this->tunedSliceNodeColors_.end())
            {
            if (!colorsNearlyEqual(current, storedIt.value()))
              {
              // User changed the layout color manually; stop adjusting this node.
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
        }
    }
}
