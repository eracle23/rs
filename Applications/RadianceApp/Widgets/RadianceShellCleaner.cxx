/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#include "RadianceShellCleaner.h"
#include "../qRadianceAppMainWindow.h"
#include "../BrandingPreferences.h"
#include "vtkSlicerConfigure.h" // For Slicer_DEFAULT_FAVORITE_MODULES

// Qt includes
#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QGuiApplication>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QScreen>
#include <QSet>
#include <QSettings>
#include <QStatusBar>
#include <QString>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

// CTK includes
#include <ctkMenuComboBox.h>

// Slicer includes
#include "qSlicerApplication.h"
#include "qSlicerLayoutManager.h"
#include "qSlicerModuleSelectorToolBar.h"
#include "qSlicerMainWindow.h"
#include <vtkMRMLLayoutNode.h>

namespace
{

static const QStringList& allowedSegmentEditorEffects()
{
  static const QStringList effects{
    QStringLiteral("Threshold"),
    QStringLiteral("Paint"),
    QStringLiteral("Erase"),
    QStringLiteral("Scissors"),
    QStringLiteral("Margin"),
    QStringLiteral("Smoothing"),
    QStringLiteral("Islands"),
    QStringLiteral("Logical operators"),
  };
  return effects;
}

static bool isSegmentEditorModule(const QString& moduleName)
{
  return moduleName == QLatin1String("SegmentEditor")
      || moduleName == QLatin1String("Segmentations");
}

static void configureSegmentEditorEffects(QWidget* root)
{
  if (!root)
    {
    return;
    }
  const QStringList& allowed = allowedSegmentEditorEffects();
  for (QWidget* w : root->findChildren<QWidget*>())
    {
    if (qstrcmp(w->metaObject()->className(), "qMRMLSegmentEditorWidget") != 0)
      {
      continue;
      }
    // 必须关闭“未列出效果仍显示”，否则 Draw/Level tracing 等会全部露出
    QMetaObject::invokeMethod(w, "setUnorderedEffectsVisible", Q_ARG(bool, false));
    QMetaObject::invokeMethod(w, "setEffectNameOrder", Q_ARG(QStringList, allowed));
    }
}

static void scheduleSegmentEditorEffectConfiguration(QMainWindow* window)
{
  if (!window)
    {
    return;
    }
  auto apply = [window]() { configureSegmentEditorEffects(window); };
  apply();
  const int delaysMs[] = {16, 32, 50, 100, 200, 400, 800, 1200};
  for (int delay : delaysMs)
    {
    QTimer::singleShot(delay, window, apply);
    }
}


static void hideAndDisableAction(QAction* a)
{
  if (!a) return;
  a->setVisible(false);
  a->setEnabled(false);
  a->setShortcuts({});
  a->setShortcut(QKeySequence());
  a->setMenuRole(QAction::NoRole);
}

static void hideActionByName(QWidget* root, const char* name)
{
  if (auto a = root->findChild<QAction*>(name))
    {
    hideAndDisableAction(a);
    }
}

static void hideActionsContainingText(QMainWindow* mw, std::initializer_list<QString> needles)
{
  if (!mw || !mw->menuBar()) return;
  const auto menus = mw->menuBar()->findChildren<QMenu*>();
  for (auto* m : menus)
    {
    for (auto* a : m->actions())
      {
      const QString t = a->text();
      for (const auto& needle : needles)
        {
        if (t.contains(needle, Qt::CaseInsensitive))
          {
          hideAndDisableAction(a);
          break;
          }
        }
      }
    }
}

static void hideDockWidgetByName(QMainWindow* mw, const char* name)
{
  if (!mw)
    {
    return;
    }
  if (auto* dock = mw->findChild<QDockWidget*>(name))
    {
    dock->hide();
    dock->setVisible(false);
    dock->setEnabled(false);
    dock->setAllowedAreas(Qt::NoDockWidgetArea);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    }
}

static void hideToolBarByName(QMainWindow* mw, const char* name)
{
  if (!mw)
    {
    return;
    }
  const QList<QToolBar*> toolBars = mw->findChildren<QToolBar*>(name);
  for (QToolBar* tb : toolBars)
    {
    if (tb && tb->objectName() == QLatin1String(name))
      {
      tb->hide();
      tb->setVisible(false);
      }
    }
}

static void hideThreeDViewMoreToolButton(QWidget* root)
{
  if (!root)
    {
    return;
    }
  for (QToolButton* btn : root->findChildren<QToolButton*>())
    {
    if (btn->objectName() != QLatin1String("MoreToolButton"))
      {
      continue;
      }
    btn->hide();
    btn->setVisible(false);
    btn->setEnabled(false);
    btn->setMaximumSize(0, 0);
    if (QMenu* menu = btn->menu())
      {
      menu->hide();
      }
    }
}

static void scheduleHideThreeDViewMoreToolButton(QWidget* root)
{
  if (!root)
    {
    return;
    }
  auto apply = [root]() { hideThreeDViewMoreToolButton(root); };
  apply();
  const int delaysMs[] = {16, 50, 100, 300, 500, 1000, 2000, 5000};
  for (int delay : delaysMs)
    {
    QTimer::singleShot(delay, root, apply);
    }
}

static void localizeSliceIntersectionTooltip(QMainWindow* mw)
{
  if (!mw)
    {
    return;
    }
  const QString sourceTextWithDot =
    QStringLiteral("Show how the other slice planes intersect each slice plane.");
  const QString sourceTextWithoutDot =
    QStringLiteral("Show how the other slice planes intersect each slice plane");
  const QString localizedText =
    QString::fromUtf8("显示其他切片平面与每个切片平面的交叉方式。");

  const auto normalize = [](const QString& text) -> QString
    {
    QString normalized = text.trimmed();
    if (normalized.endsWith(QLatin1Char('.')))
      {
      normalized.chop(1);
      }
    return normalized;
    };

  const QString sourceNormalized = normalize(sourceTextWithoutDot);

  const QList<QAction*> actions = mw->findChildren<QAction*>();
  for (QAction* action : actions)
    {
    if (!action) continue;
    const QString toolTipNormalized = normalize(action->toolTip());
    if (toolTipNormalized == sourceNormalized ||
        action->toolTip() == sourceTextWithDot ||
        action->toolTip() == sourceTextWithoutDot)
      {
      action->setToolTip(localizedText);
      action->setStatusTip(localizedText);
      action->setWhatsThis(localizedText);
      }
    }

  const QList<QToolButton*> toolButtons = mw->findChildren<QToolButton*>();
  for (QToolButton* button : toolButtons)
    {
    if (!button) continue;
    const QString toolTipNormalized = normalize(button->toolTip());
    if (toolTipNormalized == sourceNormalized ||
        button->toolTip() == sourceTextWithDot ||
        button->toolTip() == sourceTextWithoutDot)
      {
      button->setToolTip(localizedText);
      button->setStatusTip(localizedText);
      button->setWhatsThis(localizedText);
      }
    }
}

} // namespace

//-----------------------------------------------------------------------------
void RadianceShellCleaner::apply(qRadianceAppMainWindow* window)
{
  if (!window)
    {
    return;
    }

  // Help 菜单净化（隐藏 About/Docs/Tutorials/Acknowledgments/Feedback 等）
  hideActionByName(window, "HelpReportBugOrFeatureRequestAction");
  hideActionByName(window, "HelpSearchFeatureRequestsAction");
  hideActionByName(window, "HelpDocumentationAction");
  hideActionByName(window, "HelpBrowseTutorialsAction");
  hideActionByName(window, "HelpAcknowledgmentsAction");
  hideActionByName(window, "HelpAboutSlicerAppAction");

  // 文本兜底（跨版本/翻译差异）
  hideActionsContainingText(window, { "Documentation", "Tutorial", "Acknowledg", "Feedback", "Report Bug", "Feature Request" });

  // 隐藏 Python Console / Interactor 及 Error Log（含快捷键禁用）
  const char* pythonActionNames[] = {
    "ViewPythonInteractorAction",
    "ViewPythonConsoleAction",
    "WindowPythonInteractorAction",
    "WindowPythonConsoleAction"
  };
  for (auto n : pythonActionNames)
    {
    hideActionByName(window, n);
    }
  hideActionByName(window, "WindowErrorLogAction");
  hideActionsContainingText(window, { "Python Interactor", "Python Console", "Error Log" });
  hideDockWidgetByName(window, "PythonConsoleDockWidget");
  hideDockWidgetByName(window, "ErrorLogDockWidget");
  // 隐藏状态栏中的错误日志按钮，并禁用其 toggle 动作
  if (auto* errDock = window->findChild<QDockWidget*>("ErrorLogDockWidget"))
    {
    if (QAction* toggleAct = errDock->toggleViewAction())
      {
      hideAndDisableAction(toggleAct);
      }
    }
  if (QStatusBar* sb = window->statusBar())
    {
    for (QWidget* w : sb->findChildren<QToolButton*>())
      {
      if (w->inherits("QToolButton") && static_cast<QToolButton*>(w)->defaultAction())
        {
        QAction* a = static_cast<QToolButton*>(w)->defaultAction();
        if (a->toolTip().contains("Error Log", Qt::CaseInsensitive) ||
            a->text().contains("Error Log", Qt::CaseInsensitive))
          {
          w->hide();
          w->setVisible(false);
          break;
          }
        }
      }
    }

  // ========== 隐藏模块旁的搜索按钮（只保留模块下拉） ==========
  auto hideModuleSearchUI = [window]()
    {
    if (auto* selector = window->moduleSelector())
      {
      hideActionByName(selector, "ViewFindModuleAction");
      // 方法1：通过 ViewFindModuleAction 的 associatedWidgets 查找并隐藏（最可靠）
      if (auto* viewMenu = window->findChild<QMenu*>("ViewMenu"))
        {
        for (QAction* a : viewMenu->actions())
          {
          if (a->objectName() == "ViewFindModuleAction")
            {
            for (QWidget* w : a->associatedWidgets())
              {
              if (auto* tb = qobject_cast<QToolButton*>(w))
                {
                tb->hide();
                tb->setVisible(false);
                tb->setEnabled(false);
                }
              }
            hideAndDisableAction(a);
            break;
            }
          }
        }
      // 方法2：在 selector 内直接查找带 ViewFindModuleAction 的 QToolButton（兜底）
      for (QWidget* w : selector->findChildren<QToolButton*>())
        {
        if (auto* tb = qobject_cast<QToolButton*>(w))
          {
          if (tb->defaultAction() && tb->defaultAction()->objectName() == "ViewFindModuleAction")
            {
            tb->hide();
            tb->setVisible(false);
            tb->setEnabled(false);
            break;
            }
          }
        }
      // 确保 ctkMenuComboBox 内部的 Search 按钮也被隐藏
      if (auto* combo = selector->modulesMenuComboBox())
        {
        combo->setSearchIconVisible(false);
        }
      }
    };
  hideModuleSearchUI();
  localizeSliceIntersectionTooltip(window);

  // ========== 工具栏按钮隐藏 ==========
  // 隐藏扩展管理器按钮
  hideActionByName(window, "ViewExtensionsManagerAction");
  hideActionByName(window, "ExtensionsManagerAction");

  // 隐藏 Python 控制台工具栏按钮（在 DialogToolBar 上）
  if (QToolBar* dialogToolBar = window->findChild<QToolBar*>("DialogToolBar"))
    {
    for (QAction* action : dialogToolBar->actions())
      {
      hideAndDisableAction(action);
      }
    dialogToolBar->hide();
    }

  // 隐藏 Markups 快捷栏（fiducial/ruler/angle 等标注工具）
  hideToolBarByName(window, "MarkupsToolBar");

  // 3D 视图「更多」菜单（深度剥离 / 立体观看 / FPS）
  hideThreeDViewMoreToolButton(window);
  scheduleHideThreeDViewMoreToolButton(window);

  // ========== 布局菜单（视窗布局）白名单 ==========
  const QSet<QString> allowedLayoutActions = {
    "ViewLayoutConventionalAction",
    "ViewLayoutFourUpAction",
    "ViewLayoutOneUp3DAction",
    "ViewLayoutOneUpRedSliceAction",
    "ViewLayoutOneUpYellowSliceAction",
    "ViewLayoutOneUpGreenSliceAction",
  };
  const auto layoutActions = window->findChildren<QAction*>();
  for (QAction* a : layoutActions)
    {
    if (!a)
      {
      continue;
      }
    const QString name = a->objectName();
    if (!name.startsWith("ViewLayout"))
      {
      continue;
      }
    if (allowedLayoutActions.contains(name))
      {
      continue;
      }
    hideAndDisableAction(a);
    }

  // 首页左侧栏显示：首次显示窗口时切换到 DICOM，并确保左侧面板可见
  QObject::connect(window, &qSlicerMainWindow::initialWindowShown, window, [window, hideModuleSearchUI]() {
    // 延迟再次执行隐藏模块搜索，确保工具栏完全初始化后生效
    QTimer::singleShot(100, window, hideModuleSearchUI);
    // Markups 工具栏可能在模块加载后创建，延迟再次隐藏；切片交叉按钮 tooltip 汉化兜底
    QTimer::singleShot(300, window, [window]() {
      hideToolBarByName(window, "MarkupsToolBar");
      hideThreeDViewMoreToolButton(window);
      localizeSliceIntersectionTooltip(window);
      });
    QTimer::singleShot(1000, window, [window]() {
      hideThreeDViewMoreToolButton(window);
      });
    // 再次强制隐藏错误日志（restoreGUIState 可能从设置恢复了其可见状态）
    if (auto* errDock = window->findChild<QDockWidget*>("ErrorLogDockWidget"))
      {
      errDock->hide();
      errDock->setVisible(false);
      }
    // 确保视窗布局可见（常规三视图+3D），避免中央区域空白
    if (qSlicerLayoutManager* lm = qSlicerApplication::application()->layoutManager())
      {
      lm->setLayout(vtkMRMLLayoutNode::SlicerLayoutConventionalView);
      }
    // 初始化收藏模块（如果 QSettings 中没有设置，使用默认值）
    QStringList favoriteModules = QSettings().value("Modules/FavoriteModules").toStringList();
    if (favoriteModules.isEmpty())
      {
      QString defaultFavorites = QString(Slicer_DEFAULT_FAVORITE_MODULES);
      favoriteModules = defaultFavorites.split(",", Qt::SkipEmptyParts);
      for (QString& s : favoriteModules)
        {
        s = s.trimmed();
        }
      QSettings().setValue("Modules/FavoriteModules", favoriteModules);
      window->on_FavoriteModulesChanged();
      }

    if (auto selector = window->moduleSelector())
      {
      // 连接一次性信号，在模块切换完成后居中窗口
      QMetaObject::Connection* conn = new QMetaObject::Connection();
      *conn = QObject::connect(selector, &qSlicerModuleSelectorToolBar::moduleSelected, window, [window, conn]() {
        QObject::disconnect(*conn);
        delete conn;
        QTimer::singleShot(100, window, [window]() {
          if (QScreen* screen = QGuiApplication::primaryScreen())
            {
            QRect screenGeometry = screen->availableGeometry();
            int x = (screenGeometry.width() - window->width()) / 2;
            int y = (screenGeometry.height() - window->height()) / 2;
            window->move(x, y);
            }
          });
        });
      selector->selectModule("DICOM");
      }
    if (auto panel = window->findChild<QDockWidget*>("PanelDockWidget"))
      {
      panel->show();
      panel->raise();
      }

    scheduleSegmentEditorEffectConfiguration(window);
    if (auto selector = window->moduleSelector())
      {
      QObject::connect(selector, &qSlicerModuleSelectorToolBar::moduleSelected, window,
        [window](const QString& moduleName) {
          if (isSegmentEditorModule(moduleName))
            {
            scheduleSegmentEditorEffectConfiguration(window);
            }
        });
      }

    // ========== 模块下拉菜单过滤：精简为第二张图样式 ==========
    QTimer::singleShot(500, window, [window]() {
      QStringList allowedModules;
      allowedModules << "DICOM" << "Volumes" << "SegmentEditor"
                     << "Markups" << "Models";

      if (auto selector = window->moduleSelector())
        {
        if (auto comboBox = selector->findChild<ctkMenuComboBox*>())
          {
          if (auto menu = comboBox->menu())
            {
            std::function<bool(QMenu*)> filterMenu = [&](QMenu* m) -> bool {
              bool hasAllowedModule = false;
              foreach (QAction* action, m->actions())
                {
                if (action->isSeparator())
                  {
                  continue;
                  }
                if (action->menu())
                  {
                  bool subHasAllowed = filterMenu(action->menu());
                  action->setVisible(subHasAllowed);
                  if (subHasAllowed) hasAllowedModule = true;
                  }
                else
                  {
                  QString moduleName = action->data().toString();
                  if (!moduleName.isEmpty())
                    {
                    if (allowedModules.contains(moduleName))
                      {
                      action->setVisible(true);
                      hasAllowedModule = true;
                      }
                    else
                      {
                      action->setVisible(false);
                      }
                    }
                  }
                }
              return hasAllowedModule;
            };
            filterMenu(menu);
            }
          }
        }
      });
    });
}
