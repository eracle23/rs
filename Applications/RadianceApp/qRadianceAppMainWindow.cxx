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
#include "vtkSlicerConfigure.h" // For Slicer_DEFAULT_FAVORITE_MODULES

// Qt includes
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QColor>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QGuiApplication>
#include <QScreen>
#include <QFont>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QDesktopServices>
#include <QUrl>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QSize>
#include <QStringList>
#include <QStandardItem>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QUrl>
#include <QStyle>
#include <QStyleFactory>
#include <QSettings>
#include <QFile>
#include <QTextStream>
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
#include "qSlicerApplication.h"
#include "qSlicerIOManager.h"
#include "Widgets/AppLogger.h"
#include "Widgets/LicenseManager.h"
#include "Widgets/UserManager.h"
#include "Widgets/qUserManagementDialog.h"
#include "Widgets/qChangePasswordDialog.h"
#include "Widgets/DoctorAnnotationWidget.h"
#include "qSlicerCoreApplication.h"
#include "qSlicerCoreIOManager.h"
#include "qSlicerMainWindow_p.h"
#include "qSlicerModuleManager.h"
#include "qSlicerModuleFactoryManager.h"
#include "qSlicerModuleSelectorToolBar.h"
#include "qSlicerLayoutManager.h"
#include <vtkMRMLLayoutLogic.h>
#include <vtkMRMLLayoutNode.h>
#include "qSlicerSettingsStylesPanel.h"
#include "qSlicerStyle.h"
#include "qSlicerAbstractModuleWidget.h"
#include <qMRMLWidget.h>
#include "Widgets/ThemeSync.h"
#include "Widgets/SystemColorScheme.h"
#include "Widgets/SystemColorSchemeWatcher.h"
#include "Widgets/RadianceShellCleaner.h"
#include "Widgets/SliceColorAdapter.h"

// CTK includes
#include <ctkMenuComboBox.h>
#include <ctkSettingsDialog.h>
#include <ctkUtils.h>
#include <ctkVTKWidgetsUtils.h>

// VTK includes
#include <vtkNew.h>
#include <vtkMRMLMessageCollection.h>
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
    {QStringLiteral("preferences"), "toolbar_settings.svg"},
    {QStringLiteral("transforms"), "toolbar_transforms.svg"},
    {QStringLiteral("transform"), "toolbar_transforms.svg"},
    {QStringLiteral("markups"), "toolbar_markups.svg"},
    {QStringLiteral("markup"), "toolbar_markups.svg"},
    {QStringLiteral("fiducials"), "toolbar_markups.svg"},
    {QStringLiteral("models"), "toolbar_models.svg"},
    {QStringLiteral("model"), "toolbar_models.svg"},
    {QStringLiteral("elastix"), "toolbar_elastix.svg"},
    {QStringLiteral("slicerelastix"), "toolbar_elastix.svg"},
    {QStringLiteral("registration"), "toolbar_elastix.svg"}
  };

  const auto it = moduleIconMap.constFind(key);
  if (it != moduleIconMap.constEnd())
    {
    return radianceToolbarIcon(*it);
    }
  return QIcon();
}

QPalette buildDarkPalette(const QPalette& basePalette)
{
  // 使用 Slicer「Dark Slicer」默认调色板，与关闭主题切换时的外观一致
  if (QApplication* app = qApp)
    {
    if (const qSlicerStyle* slicerStyle = qobject_cast<const qSlicerStyle*>(app->style()))
      {
      return slicerStyle->standardDarkPalette();
      }
    }
  return basePalette;
}

QPalette buildLightPalette(const QPalette& basePalette)
{
  QPalette palette = basePalette;
  const QColor window(242, 244, 247);
  const QColor base(250, 250, 252);
  const QColor altBase(232, 236, 242);
  const QColor text(24, 24, 27);
  const QColor disabledText(150, 150, 150);
  const QColor highlight(42, 130, 218);
  const QColor highlightedText(255, 255, 255);
  const QColor button(240, 242, 246);

  palette.setColor(QPalette::Window, window);
  palette.setColor(QPalette::Base, base);
  palette.setColor(QPalette::AlternateBase, altBase);
  palette.setColor(QPalette::Button, button);
  palette.setColor(QPalette::WindowText, text);
  palette.setColor(QPalette::Text, text);
  palette.setColor(QPalette::ButtonText, text);
  palette.setColor(QPalette::Highlight, highlight);
  palette.setColor(QPalette::HighlightedText, highlightedText);
  palette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
  palette.setColor(QPalette::Disabled, QPalette::Text, disabledText);
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
  return palette;
}


bool isLightThemeMode(const QString& mode)
{
  return mode == QStringLiteral("light");
}

/// 断开 UI 自动连接可能产生的重复 triggered，再只连到 Radiance 槽一次。
void connectMainWindowActionOnce(
  qRadianceAppMainWindow* window,
  QAction* action,
  void (qRadianceAppMainWindow::*slot)())
{
  if (!window || !action)
    {
    return;
    }
  action->disconnect(SIGNAL(triggered()));
  QObject::connect(action, &QAction::triggered, window, slot);
}

void styleBrandHeader(QWidget* brandHeader, const QString& mode)
{
  if (!brandHeader)
    {
    return;
    }

  const bool light = isLightThemeMode(mode);
  const char* barStyle = light
    ? "#AliceTitleBar { background-color: #f2f4f7; border-bottom: 1px solid #c8c8c8; }"
    : "#AliceTitleBar { background-color: #323232; border-bottom: 1px solid #2b2b2b; }";
  const char* labelStyle = light
    ? "font-size: 18px; font-weight: 700; color: #18181b;"
    : "font-size: 18px; font-weight: 700; color: #ffffff;";
  const char* badgeStyle = light
    ? "padding: 2px 12px; border-radius: 12px; background: #2a82da; color: #ffffff; font-size: 11px; font-weight: 600;"
    : "padding: 2px 12px; border-radius: 12px; background: #3ca4ff; color: #ffffff; font-size: 11px; font-weight: 600;";

  brandHeader->setStyleSheet(barStyle);
  brandHeader->setAutoFillBackground(true);
  if (auto* label = brandHeader->findChild<QLabel*>("AliceBrandLabel"))
    {
    label->setStyleSheet(labelStyle);
    }
  if (auto* badge = brandHeader->findChild<QLabel*>("AliceWorkflowBadge"))
    {
    badge->setStyleSheet(badgeStyle);
    }
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
  
  // 确保 FavoriteModules 有默认值，否则模块加载时不会添加到工具栏
  QSettings settings;
  QStringList favModules = settings.value("Modules/FavoriteModules").toStringList();
  if (favModules.isEmpty())
    {
    // 使用编译时默认值
    QString defaultFavorites = QString(Slicer_DEFAULT_FAVORITE_MODULES);
    favModules = defaultFavorites.split(",", Qt::SkipEmptyParts);
    for (QString& s : favModules)
      {
      s = s.trimmed();
      }
    settings.setValue("Modules/FavoriteModules", favModules);
    }
  
  // 关键：在调用基类init()之前，将FavoriteModules成员变量初始化
  // 这样在模块加载时（onModuleLoaded回调），FavoriteModules列表已经有值
  this->FavoriteModules = favModules;
  
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

  if (!this->ThemeSyncHandler)
    {
    this->ThemeSyncHandler = new ThemeSync(q);
    }
  if (!this->SliceColorAdapterHandler && this->ThemeSyncHandler)
    {
    this->SliceColorAdapterHandler = new SliceColorAdapter(this->ThemeSyncHandler, q);
    }

  if (!this->SystemColorSchemeWatcherHandler)
    {
    this->SystemColorSchemeWatcherHandler = new SystemColorSchemeWatcher(q);
    QObject::connect(this->SystemColorSchemeWatcherHandler, &SystemColorSchemeWatcher::schemeChanged,
                     q, [this]() {
                       QSettings settings;
                       if (settings.value(QStringLiteral("Radiance/Theme"), QStringLiteral("dark"))
                             .toString() == QStringLiteral("system"))
                         {
                         this->applyThemeMode(QStringLiteral("system"), false);
                         }
                     });
    }

  // 在启动完成后刷新 FavoriteModules 工具栏
  // 因为某些模块（如 DICOM）可能在信号连接前就加载了
  QObject::connect(qSlicerApplication::application(), &qSlicerApplication::startupCompleted,
                   q, &qRadianceAppMainWindow::on_FavoriteModulesChanged);
}

void qRadianceAppMainWindowPrivate::applyThemeMode(const QString& mode, bool persist)
{
  if (persist)
    {
    QSettings settings;
    settings.setValue("Radiance/Theme", mode);
    }

  if (QApplication* app = qApp)
    {
    QPalette basePalette;
    if (QStyle* style = app->style())
      {
      basePalette = style->standardPalette();
      }
    else
      {
      basePalette = app->palette();
      }

    if (mode == QStringLiteral("dark"))
      {
      app->setPalette(buildDarkPalette(basePalette));
      }
    else if (mode == QStringLiteral("light"))
      {
      app->setPalette(buildLightPalette(basePalette));
      }
    else
      {
      // 跟随系统：读取 Windows 深浅色模式，而非 Slicer 默认 Dark 调色板
      if (SystemColorScheme::isDarkMode())
        {
        app->setPalette(buildDarkPalette(basePalette));
        }
      else
        {
        app->setPalette(buildLightPalette(basePalette));
        }
      }

    this->CurrentThemeMode = mode;

    if (this->ThemeSyncHandler)
      {
      this->ThemeSyncHandler->applyBranding();
      }

    Q_Q(qRadianceAppMainWindow);
    this->applyBrandHeaderTheme(q, mode);
    }
}


//-----------------------------------------------------------------------------
void qRadianceAppMainWindowPrivate::applyBrandHeaderTheme(QMainWindow* mainWindow, const QString& mode)
{
  if (!mainWindow)
    {
    return;
    }
  styleBrandHeader(mainWindow->findChild<QWidget*>(QStringLiteral("AliceTitleBar")), mode);
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
  helpAboutSlicerAppAction->setText(QString::fromUtf8("关于"));

  //----------------------------------------------------------------------------
  // 在 setupUi 前注册动作，由 connectSlotsByName 自动连到 on_*_triggered 槽（勿再手动 connect）。
  this->Superclass::setupUi(mainWindow);

  connectMainWindowActionOnce(q, helpAboutSlicerAppAction,
    &qRadianceAppMainWindow::on_HelpAboutRadianceAppAction_triggered);
  connectMainWindowActionOnce(q, this->FileSaveSceneAction,
    &qRadianceAppMainWindow::on_FileSaveSceneAction_triggered);
  connectMainWindowActionOnce(q, this->SDBSaveToDirectoryAction,
    &qRadianceAppMainWindow::on_SDBSaveToDirectoryAction_triggered);
  connectMainWindowActionOnce(q, this->SDBSaveToMRBAction,
    &qRadianceAppMainWindow::on_SDBSaveToMRBAction_triggered);

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

  // 帮助菜单：仅保留「关于」一项，点击后弹出产品版本信息
  QMenu* helpMenu = nullptr;
  if (this->HelpMenu)
    {
    this->HelpMenu->clear();
    this->HelpMenu->setObjectName("RadianceHelpMenu");
    this->HelpMenu->addAction(helpAboutSlicerAppAction);
    helpMenu = this->HelpMenu;
    if (QAction* helpMenuAction = helpMenu->menuAction())
      {
      helpMenuAction->setText(QString::fromUtf8("帮助"));
      helpMenuAction->setVisible(true);
      }
    }

  // 调试用错误日志入口已关闭，不提供「打开日志目录」「打开当前日志文件」菜单项

  //----------------------------------------------------------------------------
  // 用户菜单：当前用户、修改密码、用户管理（仅管理员）、退出登录
  //----------------------------------------------------------------------------
  QMenu* userMenu = new QMenu(QString::fromUtf8("用户"), mainWindow);
  userMenu->setObjectName("UserMenu");

  QAction* currentUserAction = new QAction(mainWindow);
  currentUserAction->setObjectName("UserCurrentUserAction");
  currentUserAction->setEnabled(false);

  QAction* changePasswordAction = new QAction(mainWindow);
  changePasswordAction->setObjectName("UserChangePasswordAction");
  changePasswordAction->setText(QString::fromUtf8("修改密码"));
  QObject::connect(changePasswordAction, &QAction::triggered, mainWindow, [mainWindow]() {
    qChangePasswordDialog dlg(mainWindow);
    dlg.exec();
  });

  QAction* userManagementAction = new QAction(mainWindow);
  userManagementAction->setObjectName("UserManagementAction");
  userManagementAction->setText(QString::fromUtf8("用户管理"));
  QObject::connect(userManagementAction, &QAction::triggered, mainWindow, [mainWindow]() {
    qUserManagementDialog dlg(mainWindow);
    dlg.exec();
  });

  QAction* logoutAction = new QAction(mainWindow);
  logoutAction->setObjectName("UserLogoutAction");
  logoutAction->setText(QString::fromUtf8("退出登录"));
  QObject::connect(logoutAction, &QAction::triggered, mainWindow, [mainWindow]() {
    if (QMessageBox::question(mainWindow, QString::fromUtf8("退出登录"),
          QString::fromUtf8("确定要退出登录吗？"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
      {
      UserManager::instance().logout();
      QApplication::quit();
      }
  });

  userMenu->addAction(currentUserAction);
  userMenu->addSeparator();
  userMenu->addAction(changePasswordAction);
  userMenu->addAction(userManagementAction);
  userMenu->addSeparator();
  userMenu->addAction(logoutAction);

  auto updateUserMenu = [currentUserAction, userManagementAction]() {
    if (UserManager::instance().isUserLoggedIn())
      {
      UserInfo user = UserManager::instance().getCurrentUser();
      QString displayName = user.fullName.isEmpty() ? user.username : user.fullName;
      currentUserAction->setText(QString::fromUtf8("当前用户: %1").arg(displayName));
      currentUserAction->setVisible(true);
      userManagementAction->setVisible(user.role == QString::fromUtf8("admin"));
      }
    else
      {
      currentUserAction->setVisible(false);
      userManagementAction->setVisible(false);
      }
  };

  // 信号驱动：用户登录/登出时立即更新菜单，无需等到菜单打开
  QObject::connect(&UserManager::instance(), &UserManager::userLoggedIn,
                   mainWindow, [updateUserMenu](const UserInfo&) { updateUserMenu(); });
  QObject::connect(&UserManager::instance(), &UserManager::userLoggedOut,
                   mainWindow, updateUserMenu);
  // 兜底：菜单展开时补刷一次（应对启动时序或外部状态变化）
  QObject::connect(userMenu, &QMenu::aboutToShow, mainWindow, updateUserMenu);
  updateUserMenu();

  // 菜单顺序：工作区 → 用户 → 帮助 → 外观（用户为从左到右第 2 项）
  if (QMenuBar* menuBar = mainWindow->menuBar())
    {
    auto firstVisibleMenuAfter = [menuBar](QAction* after) -> QAction* {
      const QList<QAction*> actions = menuBar->actions();
      const int start = after ? actions.indexOf(after) : -1;
      for (int i = start + 1; i < actions.size(); ++i)
        {
        QAction* a = actions.at(i);
        if (a && a->isVisible() && a->menu())
          {
          return a;
          }
        }
      return nullptr;
      };

    QAction* before = nullptr;
    if (this->FileMenu)
      {
      before = firstVisibleMenuAfter(this->FileMenu->menuAction());
      }
    if (!before && this->HelpMenu)
      {
      before = this->HelpMenu->menuAction();
      }
    if (before)
      {
      menuBar->insertMenu(before, userMenu);
      }
    else
      {
      menuBar->addMenu(userMenu);
      }

    if (helpMenu)
      {
      if (QAction* helpMenuAction = helpMenu->menuAction())
        {
        menuBar->removeAction(helpMenuAction);
        }
      before = firstVisibleMenuAfter(userMenu->menuAction());
      if (before)
        {
        menuBar->insertMenu(before, helpMenu);
        }
      else
        {
        menuBar->addMenu(helpMenu);
        }
      }

    if (this->AppearanceMenu)
      {
      this->AppearanceMenu->menuAction()->setText(QString::fromUtf8("外观"));
      QAction* appearanceAction = this->AppearanceMenu->menuAction();
      menuBar->removeAction(appearanceAction);
      before = helpMenu ? firstVisibleMenuAfter(helpMenu->menuAction()) : firstVisibleMenuAfter(userMenu->menuAction());
      if (before)
        {
        menuBar->insertMenu(before, this->AppearanceMenu);
        }
      else
        {
        menuBar->addMenu(this->AppearanceMenu);
        }
      }
    }

  if (this->FileMenu)
    {
    this->FileMenu->menuAction()->setText(QString::fromUtf8("工作区"));
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
  if (this->LayoutMenu)
    {
    this->LayoutMenu->menuAction()->setVisible(true);
    }
  if (this->WindowToolBarsMenu)
    {
    this->WindowToolBarsMenu->menuAction()->setVisible(true);
    }
  if (this->AppearanceMenu && RadianceBranding::themeSwitchAllowed())
    {
    this->AppearanceMenu->menuAction()->setVisible(true);

    QMenu* themeMenu = this->AppearanceMenu->findChild<QMenu*>("RadianceThemeMenu");
    if (!themeMenu)
      {
      this->AppearanceMenu->addSeparator();
      themeMenu = new QMenu(QString::fromUtf8("主题"), this->AppearanceMenu);
      themeMenu->setObjectName("RadianceThemeMenu");
      this->AppearanceMenu->addMenu(themeMenu);

      QActionGroup* themeGroup = new QActionGroup(mainWindow);
      themeGroup->setExclusive(true);

      QAction* systemThemeAction = new QAction(QString::fromUtf8("跟随系统"), mainWindow);
      systemThemeAction->setCheckable(true);
      systemThemeAction->setObjectName("ThemeSystemAction");
      themeGroup->addAction(systemThemeAction);
      themeMenu->addAction(systemThemeAction);

      QAction* lightThemeAction = new QAction(QString::fromUtf8("浅色主题"), mainWindow);
      lightThemeAction->setCheckable(true);
      lightThemeAction->setObjectName("ThemeLightAction");
      themeGroup->addAction(lightThemeAction);
      themeMenu->addAction(lightThemeAction);

      QAction* darkThemeAction = new QAction(QString::fromUtf8("深色主题"), mainWindow);
      darkThemeAction->setCheckable(true);
      darkThemeAction->setObjectName("ThemeDarkAction");
      themeGroup->addAction(darkThemeAction);
      themeMenu->addAction(darkThemeAction);

      QObject::connect(systemThemeAction, &QAction::triggered, q, [this]() {
        this->applyThemeMode(QStringLiteral("system"), true);
      });
      QObject::connect(lightThemeAction, &QAction::triggered, q, [this]() {
        this->applyThemeMode(QStringLiteral("light"), true);
      });
      QObject::connect(darkThemeAction, &QAction::triggered, q, [this]() {
        this->applyThemeMode(QStringLiteral("dark"), true);
      });
      }

    QSettings settings;
    const QString savedTheme = settings.value(QStringLiteral("Radiance/Theme"), QStringLiteral("dark")).toString();
    if (auto* systemAction = this->AppearanceMenu->findChild<QAction*>("ThemeSystemAction"))
      {
      systemAction->setChecked(savedTheme == QStringLiteral("system"));
      }
    if (auto* lightAction = this->AppearanceMenu->findChild<QAction*>("ThemeLightAction"))
      {
      lightAction->setChecked(savedTheme == QStringLiteral("light"));
      }
    if (auto* darkAction = this->AppearanceMenu->findChild<QAction*>("ThemeDarkAction"))
      {
      darkAction->setChecked(savedTheme == QStringLiteral("dark"));
      }

    this->applyThemeMode(savedTheme, false);
    }
  else if (this->AppearanceMenu)
    {
    this->AppearanceMenu->menuAction()->setVisible(false);
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

  QLabel* brandLabel = new QLabel(QString::fromUtf8("医学影像三维重建软件"));
  brandLabel->setObjectName("AliceBrandLabel");
  brandLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
  QLabel* workflowBadge = new QLabel(QString::fromUtf8("工作流"));
  workflowBadge->setObjectName("AliceWorkflowBadge");
  workflowBadge->setAlignment(Qt::AlignCenter);

  brandLayout->addWidget(brandLabel);
  brandLayout->addStretch();
  brandLayout->addWidget(workflowBadge);

  this->PanelDockWidget->setTitleBarWidget(brandHeader);
  if (QApplication* app = qApp)
    {
    brandHeader->setPalette(app->palette());
    }
  this->applyBrandHeaderTheme(mainWindow, this->CurrentThemeMode);
  this->PanelDockWidget->setWindowTitle(QString::fromUtf8("工作流"));

  // 医生批注 Dock
  QDockWidget* doctorAnnotationDock = new QDockWidget(QString::fromUtf8("医生批注"), mainWindow);
  doctorAnnotationDock->setObjectName("DoctorAnnotationDockWidget");
  DoctorAnnotationWidget* doctorAnnotationWidget = new DoctorAnnotationWidget(doctorAnnotationDock);
  doctorAnnotationDock->setWidget(doctorAnnotationWidget);
  mainWindow->addDockWidget(Qt::BottomDockWidgetArea, doctorAnnotationDock);
  doctorAnnotationDock->setVisible(true);

  // 隐藏模块面板中的帮助及致谢栏
  if (this->ModulePanel)
    {
    this->ModulePanel->setHelpAndAcknowledgmentVisible(false);
    }

  if (this->MainToolBar)
    {
    this->MainToolBar->setWindowTitle(QString::fromUtf8("数据导入导出"));
    }
  
  // 显示 DICOM 按钮（UI 文件中默认隐藏）
  if (this->LoadDICOMAction)
    {
    this->LoadDICOMAction->setVisible(true);
    }

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
void qRadianceAppMainWindow::restoreGUIState(bool force)
{
  this->Superclass::restoreGUIState(force);
  // 恢复后强制隐藏错误日志，防止从设置中恢复为可见
  if (auto* errDock = this->findChild<QDockWidget*>("ErrorLogDockWidget"))
    {
    errDock->hide();
    errDock->setVisible(false);
    }
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindow::saveGUIState(bool force)
{
  // 保存前确保错误日志已隐藏，避免将“可见”状态持久化
  if (auto* errDock = this->findChild<QDockWidget*>("ErrorLogDockWidget"))
    {
    errDock->hide();
    }
  this->Superclass::saveGUIState(force);
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindow::on_HelpAboutRadianceAppAction_triggered()
{
  const QString appName = QString::fromUtf8("医学影像三维重建软件");
  const QString releaseVersion = LicenseManager::getReleaseVersion();
  const QString fullVersion = LicenseManager::getVersionString();
  QMessageBox::about(
    this,
    QString::fromUtf8("关于"),
    QString::fromUtf8("%1\n\n软件发布版本：%2\n软件完整版本：%3")
      .arg(appName, releaseVersion, fullVersion));
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindow::on_FileSaveSceneAction_triggered()
{
  // Slicer 的“保存数据”对话框在用户关闭窗口时可能统一返回 false（即便已保存成功），
  // 因此只在明确返回 true 时提示“保存成功”；返回 false 时不弹“保存失败”，避免误报。
  const bool ok = qSlicerApplication::application()->ioManager()->openSaveDataDialog();
  if (ok)
    {
    QMessageBox::information(this,
                             QString::fromUtf8("保存成功"),
                             QString::fromUtf8("保存成功。"));
    APP_LOG_INFO(QString::fromUtf8("保存成功"));
    }
  else
    {
    // 仅记录日志，不弹窗：false 可能表示取消、关闭窗口或保存失败，无法区分
    APP_LOG_DEBUG(QString::fromUtf8("保存数据对话框已关闭（可能为取消或关闭）"));
    }
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindow::on_SDBSaveToDirectoryAction_triggered()
{
  // Same behavior as upstream, but add a success/failure popup.
  QString tempDir = qSlicerCoreApplication::application()->temporaryPath();
  QString saveDirName = QFileDialog::getExistingDirectory(
    this, tr("Slicer Data Bundle Directory (Select Empty Directory)"),
    tempDir, QFileDialog::ShowDirsOnly);
  if (saveDirName.isEmpty())
    {
    QMessageBox::warning(this,
                         QString::fromUtf8("保存失败"),
                         QString::fromUtf8("已取消保存。"));
    APP_LOG_WARNING(QString::fromUtf8("SDB 保存到目录：用户取消"));
    return;
    }

  qSlicerIO::IOProperties properties;
  // pass in a screen shot
  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
  if (layoutManager)
    {
    QWidget* widget = layoutManager->viewport();
    QImage screenShot = ctk::grabVTKWidget(widget);
    properties["screenShot"] = screenShot;
    }

  properties["fileName"] = saveDirName;
  vtkNew<vtkMRMLMessageCollection> userMessages;
  bool ok = false;
  ok = qSlicerCoreApplication::application()->coreIOManager()
         ->saveNodes(QString("SceneFile"), properties, userMessages);

  if (ok)
    {
    QMessageBox::information(this,
                             QString::fromUtf8("保存成功"),
                             QString::fromUtf8("保存成功。"));
    APP_LOG_INFO(QString::fromUtf8("SDB 保存到目录成功"));
    }
  else
    {
    QMessageBox::warning(this,
                         QString::fromUtf8("保存失败"),
                         QString::fromUtf8("保存失败。请检查路径与权限。"));
    APP_LOG_WARNING(QString::fromUtf8("SDB 保存到目录失败"));
    }
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindow::on_SDBSaveToMRBAction_triggered()
{
  QString fileName = QFileDialog::getSaveFileName(
    this, tr("Save Data Bundle File"),
    "", tr("Medical Reality Bundle (*.mrb)"));

  if (fileName.isEmpty())
    {
    QMessageBox::warning(this,
                         QString::fromUtf8("保存失败"),
                         QString::fromUtf8("已取消保存。"));
    APP_LOG_WARNING(QString::fromUtf8("SDB 保存为 MRB：用户取消"));
    return;
    }

  if (!fileName.endsWith(".mrb"))
    {
    fileName += QString(".mrb");
    }
  qSlicerIO::IOProperties properties;
  properties["fileName"] = fileName;
  vtkNew<vtkMRMLMessageCollection> userMessages;
  bool ok = false;
  ok = qSlicerCoreApplication::application()->coreIOManager()
         ->saveNodes(QString("SceneFile"), properties, userMessages);

  if (ok)
    {
    QMessageBox::information(this,
                             QString::fromUtf8("保存成功"),
                             QString::fromUtf8("保存成功。"));
    APP_LOG_INFO(QString::fromUtf8("SDB 保存为 MRB 成功"));
    }
  else
    {
    QMessageBox::warning(this,
                         QString::fromUtf8("保存失败"),
                         QString::fromUtf8("保存失败。请检查路径与权限。"));
    APP_LOG_WARNING(QString::fromUtf8("SDB 保存为 MRB 失败"));
    }
}
//-----------------------------------------------------------------------------
// 工具函数已迁移至 Widgets/RadianceShellCleaner.cxx
//-----------------------------------------------------------------------------
void qRadianceAppMainWindow::applyShellTweaks()
{
  RadianceShellCleaner::apply(this);
}

//-----------------------------------------------------------------------------
QColor qRadianceAppMainWindowPrivate::brandAccentColor() const
{
  return QColor("#2f7de0");
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
QIcon qRadianceAppMainWindowPrivate::createModuleIcon(const QIcon& baseIcon, const QColor& accentColor) const
{
  if (baseIcon.isNull())
    {
    return this->createModuleFinderIcon(accentColor);
    }
  return this->createTintedIcon(baseIcon, accentColor);
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
void qRadianceAppMainWindow::setHomeModuleCurrent()
{
  Q_D(qRadianceAppMainWindow);
  if (d->ModuleSelectorToolBar)
  {
    d->ModuleSelectorToolBar->selectModule(QStringLiteral("Welcome"));
  }
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindow::onModuleLoaded(const QString& moduleName)
{
  Q_D(qRadianceAppMainWindow);
  if (d->FavoriteModules.contains(moduleName))
    {
    qSlicerModuleManager* mm = qSlicerApplication::application()->moduleManager();
    if (mm)
      {
      qSlicerAbstractModule* module = qobject_cast<qSlicerAbstractModule*>(mm->module(moduleName));
      if (module)
        {
        QAction* action = module->action();
        if (action && action->icon().isNull())
          {
          const QColor accentColor = d->brandAccentColor();
          QIcon brandedIcon = d->brandModuleByName(moduleName, accentColor);
          if (!brandedIcon.isNull())
            {
            action->setIcon(brandedIcon);
            }
          else if (!module->icon().isNull())
            {
            action->setIcon(d->createModuleIcon(module->icon(), accentColor));
            }
          else
            {
            action->setIcon(d->createModuleFinderIcon(accentColor));
            }
          }
        }
      }
    }
  this->Superclass::onModuleLoaded(moduleName);
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindow::on_FavoriteModulesChanged()
{
  Q_D(qRadianceAppMainWindow);
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
    }

  const QColor accentColor = d->brandAccentColor();
  QIcon defaultIcon = d->createModuleFinderIcon(accentColor);
  qSlicerModuleManager* mm = qSlicerApplication::application()->moduleManager();

  if (mm)
    {
    for (const QString& moduleName : favoriteModules)
      {
      qSlicerAbstractModule* module = qobject_cast<qSlicerAbstractModule*>(mm->module(moduleName));
      if (module)
        {
        QAction* action = module->action();
        if (action && action->icon().isNull())
          {
          QIcon brandedIcon = d->brandModuleByName(moduleName, accentColor);
          if (!brandedIcon.isNull())
            {
            action->setIcon(brandedIcon);
            }
          else if (!module->icon().isNull())
            {
            action->setIcon(d->createModuleIcon(module->icon(), accentColor));
            }
          else
            {
            action->setIcon(defaultIcon);
            }
          }
        }
      }
    }

  this->Superclass::on_FavoriteModulesChanged();

  if (d->ModuleToolBar && mm)
    {
    QSet<QString> existingModules;
    for (QAction* a : d->ModuleToolBar->actions())
      {
      if (a && !a->isSeparator())
        {
        existingModules.insert(a->data().toString());
        }
      }
    for (const QString& moduleName : favoriteModules)
      {
      if (existingModules.contains(moduleName))
        {
        continue;
        }
      qSlicerAbstractModule* module = qobject_cast<qSlicerAbstractModule*>(mm->module(moduleName));
      if (!module)
        {
        continue;
        }
      QAction* action = module->action();
      if (!action)
        {
        continue;
        }
      if (action->icon().isNull())
        {
        QIcon brandedIcon = d->brandModuleByName(moduleName, accentColor);
        if (!brandedIcon.isNull())
          {
          action->setIcon(brandedIcon);
          }
        else if (!module->icon().isNull())
          {
          action->setIcon(d->createModuleIcon(module->icon(), accentColor));
          }
        else
          {
          action->setIcon(defaultIcon);
          }
        }
      d->ModuleToolBar->addAction(action);
      }
    }

  if (d->ModuleToolBar)
    {
    for (QAction* action : d->ModuleToolBar->actions())
      {
      if (!action || action->isSeparator())
        {
        continue;
        }
      const QString moduleName = action->data().toString();
      if (!moduleName.isEmpty())
        {
        QIcon brandedIcon = d->brandModuleByName(moduleName, accentColor);
        if (!brandedIcon.isNull())
          {
          action->setIcon(brandedIcon);
          }
        }
      }
    }
}
