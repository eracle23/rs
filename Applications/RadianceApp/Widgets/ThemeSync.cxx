// Minimal shell-level theme synchronizer implementation.

#include "ThemeSync.h"
#include "../BrandingPreferences.h"

#include <QApplication>
#include <QEvent>
#include <QPalette>
#include <QStyle>
#include <QTimer>
#include <QWidget>

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

  // 未开启主题切换时：保持默认界面，不修改主题
  if (!RadianceBranding::themeSwitchAllowed())
    {
    if (qApp)
      {
      qApp->setStyleSheet(QString());
      if (QStyle* style = qApp->style())
        {
        qApp->setPalette(style->standardPalette());
        }
      }
    this->applying_ = false;
    return;
    }

  // 允许主题切换时：仅刷新界面以应用 applyThemeMode 已设置的 palette
  if (qApp)
    {
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

  this->applying_ = false;

  // 通知订阅者（如 SliceColorAdapter）品牌样式已更新
  emit this->brandingApplied();
}
