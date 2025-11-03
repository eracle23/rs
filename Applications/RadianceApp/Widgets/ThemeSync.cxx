// Minimal shell-level theme synchronizer implementation.

#include "ThemeSync.h"

#include <QApplication>
#include <QEvent>
#include <QFile>
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
    if (qApp->styleSheet() != qss)
      {
      qApp->setStyleSheet(qss);
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

  this->applying_ = false;
}
