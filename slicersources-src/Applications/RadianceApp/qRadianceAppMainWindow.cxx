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
#include <QStandardItemModel>
#include <QTimer>
#include <QToolBar>
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
#include "qSlicerAboutDialog.h"
#include "qSlicerApplication.h"
#include "qSlicerIOManager.h"
#include "Widgets/AppLogger.h"
#include "qSlicerCoreApplication.h"
#include "qSlicerCoreIOManager.h"
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
    // 新增模块图标映射
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
    {"toolbar_place.svg", {"place", "fiducial", "annotation", u8"放置"}},
    {"toolbar_crosshair.svg", {"crosshair", u8"十字光标", u8"十字线"}},
    {"toolbar_home.svg", {"home", u8"主页"}},
    {"toolbar_settings.svg", {"settings", "preferences", "appearance", u8"设置", u8"首选项"}},
    // 新增模块图标规则
    {"toolbar_transforms.svg", {"transform", u8"变换", u8"变形"}},
    {"toolbar_markups.svg", {"markups", "markup", u8"标记", u8"标记点"}},
    {"toolbar_models.svg", {"models", "model", u8"模型", u8"三维模型"}},
    {"toolbar_elastix.svg", {"elastix", "registration", u8"配准", u8"弹性配准"}}
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
  
  // 在启动完成后刷新 FavoriteModules 工具栏
  // 因为某些模块（如 DICOM）可能在信号连接前就加载了
  QObject::connect(qSlicerApplication::application(), &qSlicerApplication::startupCompleted,
                   q, &qRadianceAppMainWindow::on_FavoriteModulesChanged);
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
  helpAboutSlicerAppAction->setText(QString::fromUtf8("关于 %1").arg(qSlicerApplication::application()->applicationName()));
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
  this->HelpMenu->setTitle(QString::fromUtf8("帮助"));

  QAction* helpSupportPortalAction = new QAction(mainWindow);
  helpSupportPortalAction->setObjectName("HelpSupportPortalAction");
  helpSupportPortalAction->setText(QString::fromUtf8("技术支持"));
  QObject::connect(helpSupportPortalAction, &QAction::triggered,
                   mainWindow, []()
                   {
                     QDesktopServices::openUrl(QUrl("https://radiancelabs.com/support"));
                   });

  this->HelpMenu->addAction(helpSupportPortalAction);
  this->HelpMenu->addSeparator();
  this->HelpMenu->addAction(helpAboutSlicerAppAction);

  // System log shortcuts
  QAction* openLogDirAction = new QAction(mainWindow);
  openLogDirAction->setObjectName("HelpOpenLogDirectoryAction");
  openLogDirAction->setText(QString::fromUtf8("打开日志目录"));
  QObject::connect(openLogDirAction, &QAction::triggered, mainWindow, []() {
    const QString dir = AppLogger::instance().logDirectory();
    if (!dir.isEmpty())
      {
      QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
      }
  });

  QAction* openCurrentLogAction = new QAction(mainWindow);
  openCurrentLogAction->setObjectName("HelpOpenCurrentLogFileAction");
  openCurrentLogAction->setText(QString::fromUtf8("打开当前日志文件"));
  QObject::connect(openCurrentLogAction, &QAction::triggered, mainWindow, []() {
    const QString path = AppLogger::instance().currentLogFile();
    if (!path.isEmpty())
      {
      QDesktopServices::openUrl(QUrl::fromLocalFile(path));
      }
  });

  // Put log actions into Help menu (even if Help menu is later hidden, actions remain available if you re-enable it)
  this->HelpMenu->addSeparator();
  this->HelpMenu->addAction(openLogDirAction);
  this->HelpMenu->addAction(openCurrentLogAction);

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

  QLabel* brandLabel = new QLabel(QString::fromUtf8("医学影像三维重建软件"));
  brandLabel->setObjectName("AliceBrandLabel");
  brandLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
  brandLabel->setStyleSheet("font-size: 18px; font-weight: 700; color: palette(windowText);");

  QLabel* workflowBadge = new QLabel(QString::fromUtf8("工作流"));
  workflowBadge->setObjectName("AliceWorkflowBadge");
  workflowBadge->setAlignment(Qt::AlignCenter);
  workflowBadge->setStyleSheet("padding: 2px 12px; border-radius: 12px; background: palette(highlight); color: palette(highlightedText); font-size: 11px; font-weight: 600;");

  brandLayout->addWidget(brandLabel);
  brandLayout->addStretch();
  brandLayout->addWidget(workflowBadge);
  brandHeader->setStyleSheet("#AliceTitleBar { background: palette(window); border-bottom: 1px solid palette(mid); }");

  this->PanelDockWidget->setTitleBarWidget(brandHeader);
  this->PanelDockWidget->setWindowTitle(QString::fromUtf8("工作流"));

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
  painter.drawText(badgeRect, Qt::AlignCenter, QString::fromUtf8("医学影像三维重建软件"));
  painter.end();

  about.setLogo(brandPixmap);
  about.exec();
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindow::on_FileSaveSceneAction_triggered()
{
  // Use Slicer's Save Data dialog. It returns true only if user completed the save successfully.
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
    // User may have cancelled or a save operation failed.
    QMessageBox::warning(this,
                         QString::fromUtf8("保存失败"),
                         QString::fromUtf8("保存失败或已取消。请检查保存路径、权限或文件是否被占用。"));
    APP_LOG_WARNING(QString::fromUtf8("保存失败或取消"));
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
  // 隐藏扩展管理器按钮
  hideActionByName(this, "ViewExtensionsManagerAction");
  hideActionByName(this, "ExtensionsManagerAction");
  
  // 隐藏 Python 控制台工具栏按钮（在 DialogToolBar 上）
  if (QToolBar* dialogToolBar = this->findChild<QToolBar*>("DialogToolBar"))
    {
    for (QAction* action : dialogToolBar->actions())
      {
      // 隐藏所有 DialogToolBar 上的按钮（包括扩展和Python控制台）
      hideAndDisableAction(action);
      }
    // 隐藏整个工具栏
    dialogToolBar->hide();
    }

  // ========== 布局菜单（视窗布局）白名单 ==========
  // 仅保留：
  // - 四格视窗(3D): ViewLayoutFourUpAction
  // - 3D视窗: ViewLayoutOneUp3DAction
  // - 红色视窗(水平位): ViewLayoutOneUpRedSliceAction
  // - 黄色视窗(矢状位): ViewLayoutOneUpYellowSliceAction
  // - 绿色视窗(冠状位): ViewLayoutOneUpGreenSliceAction
  //
  // 其余布局选项（包括“常规视窗(上下)”等）全部隐藏/禁用。
  const QSet<QString> allowedLayoutActions = {
    "ViewLayoutFourUpAction",
    "ViewLayoutOneUp3DAction",
    "ViewLayoutOneUpRedSliceAction",
    "ViewLayoutOneUpYellowSliceAction",
    "ViewLayoutOneUpGreenSliceAction",
  };
  const auto layoutActions = this->findChildren<QAction*>();
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
      // keep
      continue;
      }
    hideAndDisableAction(a);
    }

  // 首页左侧栏显示：首次显示窗口时切换到 DICOM，并确保左侧面板可见
  QObject::connect(this, &qSlicerMainWindow::initialWindowShown, this, [this]() {
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
      // 触发收藏模块工具栏刷新
      this->on_FavoriteModulesChanged();
      }
    
    if (auto selector = this->moduleSelector())
      {
      // 连接一次性信号，在模块切换完成后居中窗口
      QMetaObject::Connection* conn = new QMetaObject::Connection();
      *conn = QObject::connect(selector, &qSlicerModuleSelectorToolBar::moduleSelected, this, [this, conn]() {
        QObject::disconnect(*conn);
        delete conn;
        // 延迟居中，确保布局稳定
        QTimer::singleShot(100, this, [this]() {
          if (QScreen* screen = QGuiApplication::primaryScreen())
            {
            QRect screenGeometry = screen->availableGeometry();
            int x = (screenGeometry.width() - this->width()) / 2;
            int y = (screenGeometry.height() - this->height()) / 2;
            this->move(x, y);
            }
          });
        });
      selector->selectModule("DICOM");
      }
    if (auto panel = this->findChild<QDockWidget*>("PanelDockWidget"))
      {
      panel->show();
      panel->raise();
      }

    // ========== 模块下拉菜单过滤：延迟执行以确保菜单已构建 ==========
    QTimer::singleShot(500, this, [this]() {
      QStringList allowedModules;
      allowedModules << "DICOM" << "Volumes" << "SegmentEditor"
                     << "Markups" << "Models";

      if (auto selector = this->moduleSelector())
        {
        if (auto comboBox = selector->findChild<ctkMenuComboBox*>())
          {
          if (auto menu = comboBox->menu())
            {
            // 递归隐藏不在允许列表中的菜单项
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
                  // 子菜单 - 递归处理
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
                  // 忽略没有 moduleName 的项（不影响 hasAllowedModule）
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

//-----------------------------------------------------------------------------
void qRadianceAppMainWindow::onModuleLoaded(const QString& moduleName)
{
  Q_D(qRadianceAppMainWindow);
  
  // 检查是否是收藏模块，如果是且没有图标，先设置图标
  if (d->FavoriteModules.contains(moduleName))
  {
    qSlicerModuleManager* moduleManager = qSlicerApplication::application()->moduleManager();
    if (moduleManager)
    {
      qSlicerAbstractCoreModule* coreModule = moduleManager->module(moduleName);
      qSlicerAbstractModule* module = qobject_cast<qSlicerAbstractModule*>(coreModule);
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
  
  // 调用基类方法
  this->Superclass::onModuleLoaded(moduleName);
}

//-----------------------------------------------------------------------------
void qRadianceAppMainWindow::on_FavoriteModulesChanged()
{
  Q_D(qRadianceAppMainWindow);
  
  // 在调用基类之前，先给所有收藏模块的 action 设置图标
  // 因为基类的 addFavoriteModule 会跳过没有图标的模块
  QStringList favoriteModules = QSettings().value("Modules/FavoriteModules").toStringList();
  
  // 如果 FavoriteModules 为空，使用编译时默认值并写入 QSettings
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
  qSlicerModuleManager* moduleManager = qSlicerApplication::application()->moduleManager();
  
  if (moduleManager)
  {
    for (const QString& moduleName : favoriteModules)
    {
      qSlicerAbstractCoreModule* coreModule = moduleManager->module(moduleName);
      qSlicerAbstractModule* module = qobject_cast<qSlicerAbstractModule*>(coreModule);
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
  
  // 调用基类实现
  this->Superclass::on_FavoriteModulesChanged();
  
  // 检查并手动添加缺失的收藏模块（如 DICOM）
  // 因为基类会跳过没有图标的模块
  if (d->ModuleToolBar && moduleManager)
  {
    QSet<QString> existingModules;
    for (QAction* action : d->ModuleToolBar->actions())
    {
      if (action && !action->isSeparator())
      {
        existingModules.insert(action->data().toString());
      }
    }
    
    // 调试：写入文件
    QFile debugFile("D:/work/RS/dicom_debug.txt");
    if (debugFile.open(QIODevice::WriteOnly))
    {
      QTextStream ts(&debugFile);
      ts << "=== on_FavoriteModulesChanged ===\n";
      ts << "existingModules: " << existingModules.values().join(", ") << "\n";
      ts << "favoriteModules: " << favoriteModules.join(", ") << "\n";
      debugFile.close();
    }
    
    for (const QString& moduleName : favoriteModules)
    {
      if (existingModules.contains(moduleName))
      {
        continue; // 已存在
      }
      
      qSlicerAbstractCoreModule* coreModule = moduleManager->module(moduleName);
      qSlicerAbstractModule* module = qobject_cast<qSlicerAbstractModule*>(coreModule);
      
      // 调试
      if (debugFile.open(QIODevice::WriteOnly | QIODevice::Append))
      {
        QTextStream ts(&debugFile);
        ts << "Module " << moduleName << ": coreModule=" << (coreModule != nullptr) << ", module=" << (module != nullptr) << "\n";
        debugFile.close();
      }
      
      if (!module)
      {
        continue;
      }
      
      QAction* action = module->action();
      if (!action)
      {
        continue;
      }
      
      // 确保有图标
      if (action->icon().isNull())
      {
        QIcon brandedIcon = d->brandModuleByName(moduleName, accentColor);
        if (!brandedIcon.isNull())
        {
          action->setIcon(brandedIcon);
        }
        else
        {
          action->setIcon(defaultIcon);
        }
      }
      
      // 手动添加到工具栏
      d->ModuleToolBar->addAction(action);
      
      // 调试
      if (debugFile.open(QIODevice::WriteOnly | QIODevice::Append))
      {
        QTextStream ts(&debugFile);
        ts << "  -> Added " << moduleName << " to toolbar\n";
        debugFile.close();
      }
    }
  }
  
  // 再次确保工具栏上的按钮使用品牌图标
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
