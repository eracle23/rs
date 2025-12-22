/*==============================================================================

  Copyright (c) Kitware, Inc.

  See http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Julien Finet, Kitware, Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Radiance includes
#include "qRadianceAppMainWindow.h"
#include "qRadianceAppMainWindow_p.h"
#include "BrandingPreferences.h"

// Qt includes
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QColor>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFont>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QSize>
#include <QStringList>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTimer>
#include <QToolBar>
#include <QUrl>
#include <QStyle>
#include <QStyleFactory>
#include <QSettings>
#include <QFile>
#include <QTextBrowser>
#include <QTextEdit>
#include <QComboBox>
#include <QDockWidget>
#include <QKeySequence>
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <utility>

// Slicer includes
#include "qSlicerAbstractModule.h"
#include "qSlicerAboutDialog.h"
#include "qSlicerApplication.h"
#include "qSlicerMainWindow_p.h"
#include "qSlicerModuleFactoryFilterModel.h"
#include "qSlicerModuleFinderDialog.h"
#include "qSlicerModuleManager.h"
#include "qSlicerModuleFactoryManager.h"
#include "qSlicerModuleSelectorToolBar.h"
#include "qSlicerModulesListView.h"
#include "qSlicerLayoutManager.h"
#include <vtkMRMLLayoutLogic.h>
#include <vtkMRMLLayoutNode.h>
#include "qSlicerSettingsStylesPanel.h"
#include "qSlicerAbstractModuleWidget.h"
#include <qMRMLWidget.h>

// CTK includes
#include <ctkMenuComboBox.h>
#include <ctkSettingsDialog.h>
namespace
{

QString normalizedKey(const QString& source)
{
  QString key;
  key.reserve(source.size());
  for (const QChar& ch : source)
    {
    if (ch.isLetterOrNumber())
      {
      key.append(ch.toLower());
      }
    }
  return key;
}

QIcon radianceToolbarIcon(const char* alias)
{
  if (!alias || *alias == '\0')
    {
    return QIcon();
    }
  return QIcon(QStringLiteral(":/RadianceToolbar/") + QLatin1String(alias));
}

QIcon moduleIconOverride(const QString& moduleName)
{
  const QString key = normalizedKey(moduleName);
  if (key.isEmpty())
    {
    return QIcon();
    }

  static const QHash<QString, const char*> moduleIconMap{
    {QStringLiteral("home"), "toolbar_home.svg"},
    {QStringLiteral("welcome"), "toolbar_home.svg"},
    {QStringLiteral("radiancehome"), "toolbar_home.svg"},
    {QStringLiteral("data"), "toolbar_data.svg"},
    {QStringLiteral("volumes"), "toolbar_volumes.svg"},
    {QStringLiteral("volume"), "toolbar_volumes.svg"},
    {QStringLiteral("sampledata"), "toolbar_data.svg"},
    {QStringLiteral("dicom"), "toolbar_dicom.svg"},
    {QStringLiteral("dicombrowser"), "toolbar_dicom.svg"},
    {QStringLiteral("segmenteditor"), "toolbar_segment.svg"},
    {QStringLiteral("segmentations"), "toolbar_segment.svg"},
    {QStringLiteral("segmentstatistics"), "toolbar_segment.svg"},
    {QStringLiteral("volumerendering"), "toolbar_render.svg"},
    {QStringLiteral("volumerender"), "toolbar_render.svg"},
    {QStringLiteral("rendering"), "toolbar_render.svg"},
    {QStringLiteral("settings"), "toolbar_settings.svg"},
    {QStringLiteral("applicationsettings"), "toolbar_settings.svg"},
    {QStringLiteral("preferences"), "toolbar_settings.svg"}
  };

  const auto it = moduleIconMap.constFind(key);
  if (it != moduleIconMap.constEnd())
    {
    return radianceToolbarIcon(*it);
    }
  return QIcon();
}

struct ActionIconRule
{
  const char* iconAlias;
  std::initializer_list<const char*> needles;
};

const QHash<QString, const char*>& actionIconObjectMap()
{
  static const QHash<QString, const char*> map{
    {QStringLiteral("fileadddataaction"), "toolbar_add_data.svg"},
    {QStringLiteral("fileloaddataaction"), "toolbar_add_data.svg"},
    {QStringLiteral("fileimportsceneaction"), "toolbar_add_data.svg"},
    {QStringLiteral("fileloadsceneaction"), "toolbar_add_data.svg"},
    {QStringLiteral("fileaddvolumeaction"), "toolbar_add_data.svg"},
    {QStringLiteral("fileaddtransformaction"), "toolbar_add_data.svg"},
    {QStringLiteral("filefavoritemodulesaction"), "toolbar_data.svg"},
    {QStringLiteral("loaddicomaction"), "toolbar_dicom.svg"},
    {QStringLiteral("filesavesceneaction"), "toolbar_save.svg"},
    {QStringLiteral("sdbsavetodirectoryaction"), "toolbar_save.svg"},
    {QStringLiteral("sdbsavetomrbaction"), "toolbar_save.svg"},
    {QStringLiteral("editundoaction"), "toolbar_undo.svg"},
    {QStringLiteral("editredoaction"), "toolbar_redo.svg"},
    {QStringLiteral("modulehomeaction"), "toolbar_home.svg"},
    {QStringLiteral("editapplicationsettingsaction"), "toolbar_settings.svg"},
    {QStringLiteral("viewextensionsmanageraction"), "toolbar_extensions.svg"},
    {QStringLiteral("adjustviewaction"), "toolbar_view_control.svg"},
    {QStringLiteral("adjustwindowlevelaction"), "toolbar_window_level.svg"},
    {QStringLiteral("placewidgetaction"), "toolbar_place.svg"},
    {QStringLiteral("toolbaraction"), "toolbar_place.svg"},
    {QStringLiteral("placewidgettoolbaraction"), "toolbar_place.svg"}
  };
  return map;
}

QIcon actionIconOverride(QAction* action)
{
  if (!action)
    {
    return QIcon();
    }

  const QStringList haystacks{
    action->objectName(),
    action->text(),
    action->toolTip(),
    action->iconText(),
    action->whatsThis(),
    action->statusTip()
  };

  const QString objectKey = normalizedKey(action->objectName());
  if (!objectKey.isEmpty())
    {
    const char* alias = actionIconObjectMap().value(objectKey, nullptr);
    if (alias)
      {
      return radianceToolbarIcon(alias);
      }
    }

  QStringList normalizedHaystacks;
  normalizedHaystacks.reserve(haystacks.size());
  for (const QString& field : haystacks)
    {
    normalizedHaystacks << normalizedKey(field);
    }

  auto matches = [&haystacks, &normalizedHaystacks](const char* needle) -> bool
    {
      if (!needle || *needle == '\0')
        {
        return false;
        }
      const QString needleStr = QString::fromUtf8(needle);
      const QString needleNormalized = normalizedKey(needleStr);
      for (int i = 0; i < haystacks.size(); ++i)
        {
        const QString& field = haystacks.at(i);
        if (!field.isEmpty() && field.contains(needleStr, Qt::CaseInsensitive))
          {
          return true;
          }
        if (!needleNormalized.isEmpty())
          {
          const QString& normalizedField = normalizedHaystacks.at(i);
          if (!normalizedField.isEmpty() && normalizedField.contains(needleNormalized))
            {
            return true;
            }
          }
        }
      return false;
    };

  static const ActionIconRule kActionIconRules[] = {
    {"toolbar_add_data.svg", {
      "add data", "open data", "load data", "import scene", "load scene",
      "adddataset", "addvolume", "add transform", "addtransform",
      u8"添加数据", u8"加载数据", u8"导入场景", u8"加载场景", u8"添加体数据", u8"添加变换"
    }},
    {"toolbar_dicom.svg", {"dicom", u8"DICOM", u8"影像"}},
    {"toolbar_save.svg", {
      "save scene", "savescene", "save data", "savedata",
      u8"保存", u8"保存场景", u8"保存数据"
    }},
    {"toolbar_undo.svg", {"undo", u8"撤销"}},
    {"toolbar_redo.svg", {"redo", u8"重做"}},
    {"toolbar_snapshot.svg", {
      "screenshot", "screen capture", "capture view", "snapshot",
      u8"截图", u8"屏幕截图", u8"抓图", u8"捕获视图"
    }},
    {"toolbar_scene_view.svg", {"scene view", "sceneview", u8"场景视图", u8"场景快照"}},
    {"toolbar_extensions.svg", {"extension", "extensions manager", "extension manager", u8"扩展"}},
    {"toolbar_layout.svg", {"layout", u8"布局", u8"视图布局"}},
    {"toolbar_view_control.svg", {"adjust view", "view transform", "view navigation", u8"视图控制", u8"视图调整"}},
    {"toolbar_window_level.svg", {"window/level", "window level", "windowlevel", u8"窗宽", u8"窗位"}},
    {"toolbar_place.svg", {"place", "markups", "fiducial", "annotation", u8"放置", u8"标记"}},
    {"toolbar_crosshair.svg", {"crosshair", u8"十字光标", u8"十字线"}},
    {"toolbar_home.svg", {"home", u8"主页"}},
    {"toolbar_settings.svg", {"settings", "preferences", "appearance", u8"设置", u8"首选项"}}
  };

  for (const ActionIconRule& rule : kActionIconRules)
    {
    for (const char* needle : rule.needles)
      {
      if (matches(needle))
        {
        return radianceToolbarIcon(rule.iconAlias);
        }
      }
    }

  return QIcon();
}

} // namespace

//-----------------------------------------------------------------------------
// qRadianceAppMainWindowPrivate methods

qRadianceAppMainWindowPrivate::qRadianceAppMainWindowPrivate(qRadianceAppMainWindow& object)
  : Superclass(object)
{
}

//-----------------------------------------------------------------------------
qRadianceAppMainWindowPrivate::~qRadianceAppMainWindowPrivate()
{
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindowPrivate::init()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
  QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
  Q_Q(qRadianceAppMainWindow);
  this->Superclass::init();

  // 清空任何遗留的全局样式与调色板，确保完全回到 Slicer 默认主题
  if (QApplication* app = qApp)
    {
    app->setStyleSheet(QString());
    if (QStyle* style = app->style())
      {
      app->setPalette(style->standardPalette());
      }
    }
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindowPrivate::setupUi(QMainWindow * mainWindow)
{
  qSlicerApplication * app = qSlicerApplication::application();
  Q_Q(qRadianceAppMainWindow);

  //----------------------------------------------------------------------------
  // Add actions
  //----------------------------------------------------------------------------
  QAction* helpAboutSlicerAppAction = new QAction(mainWindow);
  helpAboutSlicerAppAction->setObjectName("HelpAboutRadianceAppAction");
  helpAboutSlicerAppAction->setText(qRadianceAppMainWindow::tr("About %1").arg(qSlicerApplication::application()->applicationName()));
  QObject::connect(helpAboutSlicerAppAction, &QAction::triggered,
                   q, &qRadianceAppMainWindow::on_HelpAboutRadianceAppAction_triggered);

  //----------------------------------------------------------------------------
  // Calling "setupUi()" after adding the actions above allows the call
  // to "QMetaObject::connectSlotsByName()" done in "setupUi()" to
  // successfully connect each slot with its corresponding action.
  this->Superclass::setupUi(mainWindow);

  // Hide Slicer-branded help/links and Extensions Manager entry (shell only)
  auto hideByName = [mainWindow](const char* name)
  {
    if (auto* a = mainWindow->findChild<QAction*>(name)) { a->setVisible(false); }
  };
  hideByName("HelpVisitSlicerForumAction");
  hideByName("HelpReportBugOrFeatureRequestAction");
  hideByName("HelpSlicerPublicationsAction");
  hideByName("HelpAboutSlicerAppAction");
  hideByName("ExtensionsManagerAction");

  // Fallback: hide by action text containing keywords
  const auto actions = mainWindow->findChildren<QAction*>();
  for (auto* a : actions)
    {
    const QString t = a->text();
    if (t.contains("Slicer", Qt::CaseInsensitive) ||
        t.contains("Extensions Manager", Qt::CaseInsensitive))
      {
      a->setVisible(false);
      }
    }

  // Add Help Menu Action
  this->HelpMenu->clear();
  this->HelpMenu->setTitle(qRadianceAppMainWindow::tr("Support"));

  QAction* helpSupportPortalAction = new QAction(mainWindow);
  helpSupportPortalAction->setObjectName("HelpSupportPortalAction");
  helpSupportPortalAction->setText(qRadianceAppMainWindow::tr("Support Portal"));
  QObject::connect(helpSupportPortalAction, &QAction::triggered,
                   mainWindow, []()
                   {
                     QDesktopServices::openUrl(QUrl("https://radiancelabs.com/support"));
                   });

  this->HelpMenu->addAction(helpSupportPortalAction);
  this->HelpMenu->addSeparator();
  this->HelpMenu->addAction(helpAboutSlicerAppAction);

  if (this->FileMenu)
    {
    this->FileMenu->menuAction()->setText(qRadianceAppMainWindow::tr("Workspace"));
    }
  // 隐藏 Edit 菜单
  if (this->EditMenu)
    {
    this->EditMenu->menuAction()->setVisible(false);
    }
  // 隐藏 View 菜单
  if (this->ViewMenu)
    {
    this->ViewMenu->menuAction()->setVisible(false);
    }
  // 隐藏 Support 菜单（原 Help 菜单）
  if (this->HelpMenu)
    {
    this->HelpMenu->menuAction()->setVisible(false);
    }
  if (this->LayoutMenu)
    {
    this->LayoutMenu->menuAction()->setVisible(true);
    }
  if (this->WindowToolBarsMenu)
    {
    this->WindowToolBarsMenu->menuAction()->setVisible(true);
    }
  if (this->AppearanceMenu)
    {
    // Show Appearance menu so users can switch themes (e.g., Dark/Light)
    this->AppearanceMenu->menuAction()->setVisible(true);
    }

  // 保持上游设置面板行为与命名，不做品牌重命名，简化设计。

  //----------------------------------------------------------------------------
  // Configure
  //----------------------------------------------------------------------------
  mainWindow->setWindowIcon(QIcon(":/Icons/Medium/DesktopIcon.png"));

  QWidget* brandHeader = new QWidget();
  brandHeader->setObjectName("AliceTitleBar");
  auto* brandLayout = new QHBoxLayout(brandHeader);
  brandLayout->setContentsMargins(12, 8, 12, 8);
  brandLayout->setSpacing(10);

  QLabel* brandLabel = new QLabel(qRadianceAppMainWindow::tr("Alice Studio"));
  brandLabel->setObjectName("AliceBrandLabel");
  brandLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
  brandLabel->setStyleSheet("font-size: 18px; font-weight: 700; color: palette(windowText);");

  QLabel* workflowBadge = new QLabel(qRadianceAppMainWindow::tr("Workflow"));
  workflowBadge->setObjectName("AliceWorkflowBadge");
  workflowBadge->setAlignment(Qt::AlignCenter);
  workflowBadge->setStyleSheet("padding: 2px 12px; border-radius: 12px; background: palette(highlight); color: palette(highlightedText); font-size: 11px; font-weight: 600;");

  brandLayout->addWidget(brandLabel);
  brandLayout->addStretch();
  brandLayout->addWidget(workflowBadge);
  brandHeader->setStyleSheet("#AliceTitleBar { background: palette(window); border-bottom: 1px solid palette(mid); }");

  this->PanelDockWidget->setTitleBarWidget(brandHeader);
  this->PanelDockWidget->setWindowTitle(qRadianceAppMainWindow::tr("Workflow"));

  if (this->MainToolBar)
    {
    this->MainToolBar->setWindowTitle(qRadianceAppMainWindow::tr("Data I/O"));
    }

  this->applyToolbarBranding();

  // 布局注册与默认模块交由 DefaultSettings.ini 与 Slicer 机制处理。

  // 保持 Welcome 文案为上游默认，不在运行时替换，减少耦合。

  // Hide the menus
  //this->menubar->setVisible(false);
  //this->FileMenu->setVisible(false);
  //this->EditMenu->setVisible(false);
  //this->ViewMenu->setVisible(false);
  //this->LayoutMenu->setVisible(false);
  //this->HelpMenu->setVisible(false);

  // 不追加全局样式，让 3D 控制条背景交由主题样式处理。

  // 统一进行壳层净化与默认显示调整
  q->applyShellTweaks();
}

//-----------------------------------------------------------------------------
// qRadianceAppMainWindow methods

//-----------------------------------------------------------------------------
qRadianceAppMainWindow::qRadianceAppMainWindow(QWidget* windowParent)
  : Superclass(new qRadianceAppMainWindowPrivate(*this), windowParent)
{
  Q_D(qRadianceAppMainWindow);
  d->init();
}

//-----------------------------------------------------------------------------
qRadianceAppMainWindow::qRadianceAppMainWindow(
  qRadianceAppMainWindowPrivate* pimpl, QWidget* windowParent)
  : Superclass(pimpl, windowParent)
{
  // init() is called by derived class.
}

//-----------------------------------------------------------------------------
qRadianceAppMainWindow::~qRadianceAppMainWindow()
{
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindow::on_HelpAboutRadianceAppAction_triggered()
{
  if (RadianceBranding::nativeStyleEnabled())
    {
    qSlicerAboutDialog about(this);
    about.exec();
    return;
    }

  qSlicerAboutDialog about(this);
  constexpr int brandWidth = 480;
  constexpr int brandHeight = 180;
  QPixmap brandPixmap(brandWidth, brandHeight);
  brandPixmap.fill(Qt::transparent);

  QPainter painter(&brandPixmap);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const QRectF badgeRect(40, brandHeight / 2.0 - 36.0, brandWidth - 80.0, 72.0);
  const QColor accentColor("#5468ff");
  painter.setBrush(accentColor);
  painter.setPen(Qt::NoPen);
  painter.drawRoundedRect(badgeRect, 36.0, 36.0);

  painter.setPen(Qt::white);
  QFont brandFont = painter.font();
  brandFont.setPointSize(28);
  brandFont.setBold(true);
  painter.setFont(brandFont);
  painter.drawText(badgeRect, Qt::AlignCenter, tr("Alice Studio"));
  painter.end();

  about.setLogo(brandPixmap);
  about.exec();
}

//-----------------------------------------------------------------------------
// 工具函数（仅在本翻译单元可见）
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

//-----------------------------------------------------------------------------
void qRadianceAppMainWindow::applyShellTweaks()
{
  // Help 菜单净化（隐藏 About/Docs/Tutorials/Acknowledgments/Feedback 等）
  hideActionByName(this, "HelpReportBugOrFeatureRequestAction");
  hideActionByName(this, "HelpSearchFeatureRequestsAction");
  hideActionByName(this, "HelpDocumentationAction");
  hideActionByName(this, "HelpBrowseTutorialsAction");
  hideActionByName(this, "HelpAcknowledgmentsAction");
  hideActionByName(this, "HelpAboutSlicerAppAction");

  // 文本兜底（跨版本/翻译差异）
  hideActionsContainingText(this, { "Documentation", "Tutorial", "Acknowledg", "About", "Feedback", "Report Bug", "Feature Request" });

  // 隐藏 Python Console / Interactor 及 Error Log（含快捷键禁用）
  const char* pythonActionNames[] = {
    "ViewPythonInteractorAction",
    "ViewPythonConsoleAction",
    "WindowPythonInteractorAction",
    "WindowPythonConsoleAction"
  };
  for (auto n : pythonActionNames)
    {
    hideActionByName(this, n);
    }
  hideActionByName(this, "WindowErrorLogAction");
  hideActionsContainingText(this, { "Python Interactor", "Python Console", "Error Log" });
  hideDockWidgetByName(this, "PythonConsoleDockWidget");
  hideDockWidgetByName(this, "ErrorLogDockWidget");

  // ========== 工具栏按钮隐藏 ==========
  // 隐藏 MainToolBar 中的 "Load Data" 按钮
  hideActionByName(this, "FileLoadDataAction");
  hideActionByName(this, "FileAddDataAction");

  // 隐藏扩展管理器按钮（菜单和工具栏中的）
  hideActionByName(this, "ViewExtensionsManagerAction");
  hideActionByName(this, "ExtensionsManagerAction");

  // 首页左侧栏显示 DICOM：首次显示窗口时切换到 DICOM，并确保左侧面板可见
  QObject::connect(this, &qSlicerMainWindow::initialWindowShown, this, [this]() {
    if (auto selector = this->moduleSelector())
      {
      selector->selectModule("DICOM");
      }
    if (auto panel = this->findChild<QDockWidget*>("PanelDockWidget"))
      {
      panel->show();
      panel->raise();
      }

    // ========== 模块过滤：只显示指定的模块 ==========
    QStringList allowedModules;
    allowedModules << "DICOM" << "Volumes" << "VolumeRendering"
                   << "SegmentEditor" << "Transforms" << "Markups"
                   << "Models" << "Elastix";

    qSlicerApplication* app = qSlicerApplication::application();
    if (app && app->moduleManager())
      {
      qSlicerModuleFactoryManager* factoryManager = app->moduleManager()->factoryManager();
      if (factoryManager)
        {
        QStringList allModules = factoryManager->instantiatedModuleNames();
        QStringList modulesToHide;
        foreach (const QString& moduleName, allModules)
          {
          if (!allowedModules.contains(moduleName))
            {
            modulesToHide << moduleName;
            }
          }
        // 设置隐藏的模块
        factoryManager->setModulesToIgnore(modulesToHide);
        }
      }

    // 刷新模块选择器
    if (auto selector = this->moduleSelector())
      {
      if (auto comboBox = selector->findChild<ctkMenuComboBox*>())
        {
        if (auto menu = comboBox->menu())
          {
          // 隐藏不在允许列表中的菜单项
          foreach (QAction* action, menu->actions())
            {
            if (action->menu())
              {
              // 子菜单 - 检查是否需要隐藏
              bool hasVisibleChild = false;
              foreach (QAction* subAction, action->menu()->actions())
                {
                QString moduleName = subAction->data().toString();
                if (allowedModules.contains(moduleName))
                  {
                  hasVisibleChild = true;
                  subAction->setVisible(true);
                  }
                else if (!moduleName.isEmpty())
                  {
                  subAction->setVisible(false);
                  }
                }
              action->setVisible(hasVisibleChild);
              }
            else
              {
              QString moduleName = action->data().toString();
              if (!moduleName.isEmpty() && !allowedModules.contains(moduleName))
                {
                action->setVisible(false);
                }
              }
            }
          }
        }
      }
  });
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindowPrivate::applyToolbarBranding()
{
  if (RadianceBranding::nativeStyleEnabled())
    {
    return;
    }

  const QColor accentColor = this->brandAccentColor();

  auto tintToolbar = [this, accentColor](QToolBar* toolbar, Qt::ToolButtonStyle style, bool applyStyle)
    {
    if (!toolbar)
      {
      return;
      }

    toolbar->setIconSize(QSize(28, 28));
    if (applyStyle)
      {
      toolbar->setToolButtonStyle(style);
      }

    const QIcon moduleFinderIcon = this->createModuleFinderIcon(accentColor);

    for (QAction* action : toolbar->actions())
      {
      if (!action || action->isSeparator())
        {
        continue;
        }
      if (action->objectName() == QStringLiteral("ViewFindModuleAction"))
        {
        action->setIcon(moduleFinderIcon);
        if (!action->property("RadianceFinderBrandingConnected").toBool())
          {
          action->setProperty("RadianceFinderBrandingConnected", true);
          QObject::connect(action, &QAction::triggered, this->q_func(), [this, accentColor]()
            {
              QTimer::singleShot(0, this->q_func(), [this, accentColor]()
                {
                  this->brandAnyVisibleModuleFinder(accentColor);
                });
            });
          }
        continue;
        }
      const QIcon overrideIcon = actionIconOverride(action);
      if (!overrideIcon.isNull())
        {
        action->setIcon(overrideIcon);
        continue;
        }
      const QIcon originalIcon = action->icon();
      if (originalIcon.isNull())
        {
        continue;
        }
      action->setIcon(this->createModuleIcon(originalIcon, accentColor));
      }
    };

  tintToolbar(this->MainToolBar, Qt::ToolButtonTextUnderIcon, /*applyStyle=*/true);
  tintToolbar(this->ModuleToolBar, Qt::ToolButtonIconOnly, /*applyStyle=*/false);
  tintToolbar(this->UndoRedoToolBar, Qt::ToolButtonIconOnly, /*applyStyle=*/false);
  tintToolbar(this->ViewToolBar, Qt::ToolButtonIconOnly, /*applyStyle=*/false);
  tintToolbar(this->ViewersToolBar, Qt::ToolButtonIconOnly, /*applyStyle=*/false);
  tintToolbar(this->MouseModeToolBar, Qt::ToolButtonIconOnly, /*applyStyle=*/false);
  tintToolbar(this->DialogToolBar, Qt::ToolButtonIconOnly, /*applyStyle=*/false);
  tintToolbar(this->LayoutToolBar, Qt::ToolButtonIconOnly, /*applyStyle=*/false);

  this->brandModuleSelectorMenu(accentColor);
  this->brandAllModules(accentColor);

  if (qSlicerApplication* app = qSlicerApplication::application())
    {
    if (qSlicerModuleManager* moduleManager = app->moduleManager())
      {
      QObject::connect(
        moduleManager, &qSlicerModuleManager::moduleLoaded,
        this->q_func(),
        [this, accentColor](const QString& moduleName)
          {
            this->brandModuleByName(moduleName, accentColor);
            this->brandAnyVisibleModuleFinder(accentColor);
          });
      }
    }
}

//-----------------------------------------------------------------------------
QColor qRadianceAppMainWindowPrivate::brandAccentColor() const
{
  return QColor("#2f7de0");
}

//-----------------------------------------------------------------------------
QIcon qRadianceAppMainWindowPrivate::createModuleIcon(const QIcon& baseIcon, const QColor& accentColor) const
{
  if (baseIcon.isNull())
    {
    return this->createModuleFinderIcon(accentColor);
    }
  return this->createTintedIcon(baseIcon, accentColor);
}

//-----------------------------------------------------------------------------
QIcon qRadianceAppMainWindowPrivate::createTintedIcon(const QIcon& source, const QColor& tint) const
{
  if (source.isNull())
    {
    return source;
    }

  const QList<QIcon::Mode> modes{QIcon::Normal, QIcon::Active, QIcon::Disabled, QIcon::Selected};
  const QList<QIcon::State> states{QIcon::Off, QIcon::On};
  QIcon tintedIcon;

  for (QIcon::Mode mode : modes)
    {
    for (QIcon::State state : states)
      {
      QList<QSize> sizes = source.availableSizes(mode, state);
      if (sizes.isEmpty())
        {
        sizes << QSize(16, 16) << QSize(24, 24) << QSize(32, 32);
        }

      for (const QSize& size : std::as_const(sizes))
        {
        if (!size.isValid())
          {
          continue;
          }

        QPixmap basePixmap = source.pixmap(size, mode, state);
        if (basePixmap.isNull())
          {
          continue;
          }

        QPixmap tintedPixmap(size);
        tintedPixmap.fill(Qt::transparent);

        QPainter painter(&tintedPixmap);
        painter.drawPixmap(0, 0, basePixmap);

        QColor modeTint = tint;
        if (mode == QIcon::Disabled)
          {
          modeTint.setAlphaF(0.35);
          modeTint = modeTint.lighter(150);
          }
        else
          {
          modeTint.setAlphaF(0.85);
          }

        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(tintedPixmap.rect(), modeTint);
        painter.end();

        tintedIcon.addPixmap(tintedPixmap, mode, state);
        }
      }
    }

  return tintedIcon.isNull() ? source : tintedIcon;
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindowPrivate::brandModuleSelectorMenu(const QColor& accentColor)
{
  if (!this->ModuleSelectorToolBar)
    {
    return;
    }

  ctkMenuComboBox* moduleCombo = this->ModuleSelectorToolBar->modulesMenuComboBox();
  if (!moduleCombo)
    {
    return;
    }
  moduleCombo->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

  auto tintActionIcon = [this, accentColor](QAction* action)
    {
      if (!action)
        {
        return;
        }
      const QString moduleName = action->data().toString();
      if (!moduleName.isEmpty())
        {
        QIcon moduleIcon = this->brandModuleByName(moduleName, accentColor);
        if (!moduleIcon.isNull())
          {
          action->setIcon(moduleIcon);
          }
        return;
        }
      const QIcon originalIcon = action->icon();
      if (originalIcon.isNull())
        {
        return;
        }
      action->setIcon(this->createModuleIcon(originalIcon, accentColor));
    };

  std::function<void(QMenu*)> tintMenuRecursively;
  tintMenuRecursively = [&](QMenu* menu)
    {
      if (!menu)
        {
        return;
        }
      const QList<QAction*> actions = menu->actions();
      for (QAction* action : actions)
        {
        tintActionIcon(action);
        if (QMenu* subMenu = action->menu())
          {
          tintMenuRecursively(subMenu);
          }
        }
    };

  tintMenuRecursively(moduleCombo->menu());
}

//-----------------------------------------------------------------------------
QIcon qRadianceAppMainWindowPrivate::createModuleFinderIcon(const QColor& accentColor) const
{
  const QList<int> baseSizes{16, 20, 24, 28, 32};
  QIcon icon;

  for (const int size : baseSizes)
    {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen pen(accentColor);
    pen.setWidthF(std::max(1.0, size * 0.12));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);

    const qreal radius = size * 0.32;
    const QPointF center(size * 0.4, size * 0.4);
    painter.drawEllipse(center, radius, radius);

    const QPointF handleStart(size * 0.62, size * 0.62);
    const QPointF handleEnd(size * 0.82, size * 0.82);
    painter.drawLine(handleStart, handleEnd);
    painter.end();

    icon.addPixmap(pixmap);
    }

  return icon;
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindowPrivate::brandAnyVisibleModuleFinder(const QColor& accentColor)
{
  const QWidgetList topLevels = QApplication::topLevelWidgets();
  for (QWidget* widget : topLevels)
    {
    if (auto finderDialog = qobject_cast<qSlicerModuleFinderDialog*>(widget))
      {
      this->brandModuleFinderDialog(finderDialog, accentColor);
      }
    }
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindowPrivate::brandModuleFinderDialog(qSlicerModuleFinderDialog* dialog, const QColor& accentColor)
{
  if (!dialog)
    {
    return;
    }

  qSlicerModulesListView* listView = dialog->findChild<qSlicerModulesListView*>("ModuleListView");
  if (!listView)
    {
    return;
    }

  this->brandModulesListView(listView, accentColor);
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindowPrivate::brandModulesListView(qSlicerModulesListView* listView, const QColor& accentColor)
{
  if (!listView)
    {
    return;
    }

  qSlicerModuleFactoryFilterModel* filterModel = listView->filterModel();
  if (!filterModel)
    {
    return;
    }

  QStandardItemModel* model = qobject_cast<QStandardItemModel*>(filterModel->sourceModel());
  if (!model)
    {
    return;
    }

  qSlicerApplication* app = qSlicerApplication::application();
  if (!app)
    {
    return;
    }

  qSlicerModuleManager* moduleManager = app->moduleManager();
  if (!moduleManager)
    {
    return;
    }

  for (int row = 0; row < model->rowCount(); ++row)
    {
    QStandardItem* item = model->item(row);
    if (!item)
      {
      continue;
      }
    const QString moduleName = item->data(qSlicerModuleFactoryFilterModel::ModuleNameRole).toString();
    if (moduleName.isEmpty())
      {
      continue;
      }
    QIcon brandedIcon = this->brandModuleByName(moduleName, accentColor);
    if (brandedIcon.isNull())
      {
      continue;
      }
    bool block = model->blockSignals(true);
    item->setIcon(brandedIcon);
    model->blockSignals(block);
    }
}

//-----------------------------------------------------------------------------
QIcon qRadianceAppMainWindowPrivate::brandModuleByName(const QString& moduleName, const QColor& accentColor)
{
  qSlicerApplication* app = qSlicerApplication::application();
  if (!app)
    {
    return QIcon();
    }

  qSlicerModuleManager* moduleManager = app->moduleManager();
  if (!moduleManager)
    {
    return QIcon();
    }

  qSlicerAbstractCoreModule* coreModule = moduleManager->module(moduleName);
  auto module = qobject_cast<qSlicerAbstractModule*>(coreModule);
  if (!module)
    {
    return QIcon();
    }

  QIcon brandedIcon = moduleIconOverride(moduleName);
  if (brandedIcon.isNull())
    {
    brandedIcon = this->createModuleIcon(module->icon(), accentColor);
    }
  if (QAction* moduleAction = module->action())
    {
    moduleAction->setIcon(brandedIcon);
    moduleAction->setIconVisibleInMenu(true);
    }

  return brandedIcon;
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindowPrivate::brandAllModules(const QColor& accentColor)
{
  qSlicerApplication* app = qSlicerApplication::application();
  if (!app)
    {
    return;
    }

  qSlicerModuleManager* moduleManager = app->moduleManager();
  if (!moduleManager)
    {
    return;
    }

  const QStringList moduleNames = moduleManager->modulesNames();
  for (const QString& moduleName : moduleNames)
    {
    this->brandModuleByName(moduleName, accentColor);
    }
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindow::setHomeModuleCurrent()
{
  Q_D(qRadianceAppMainWindow);
  if (d->ModuleSelectorToolBar)
  {
    d->ModuleSelectorToolBar->selectModule(QStringLiteral("Welcome"));
  }
}
