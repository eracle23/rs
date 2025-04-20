/*==============================================================================

  Copyright (c) Kitware, Inc.

  See http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware, Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Radiance includes
#include "qRadianceAppMainWindow.h"
#include "Widgets/qAppStyle.h"
#include "Widgets/AppLogger.h"

// Slicer includes
#include "qSlicerApplication.h"
#include "qSlicerApplicationHelper.h"
#include "vtkSlicerConfigure.h" // For Slicer_MAIN_PROJECT_APPLICATION_NAME
#include "vtkSlicerVersionConfigure.h" // For Slicer_MAIN_PROJECT_VERSION_FULL

// Qt includes
#include <QCoreApplication>
#include <QGuiApplication>
#include <QFont>
#include <QLinearGradient>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QVariant>
#include <QMetaType>
#include <QTextCodec>
#include <QDir>
// Theme/Settings
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QMutex>

namespace {

// ----------------------------------------------------------------------------
// Forward Qt logs to AppLogger (and still allow default handlers if needed).
void radianceQtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
  // Avoid recursion if logger itself logs through Qt.
  static thread_local bool inHandler = false;
  if (inHandler)
    {
    return;
    }
  inHandler = true;

  QString category = "Qt";
  if (context.category && context.category[0] != '\0')
    {
    category = QString("Qt/%1").arg(QString::fromUtf8(context.category));
    }

  switch (type)
    {
    case QtDebugMsg:
      AppLogger::instance().debug(msg, category);
      break;
    case QtInfoMsg:
      AppLogger::instance().info(msg, category);
      break;
    case QtWarningMsg:
      AppLogger::instance().warning(msg, category);
      break;
    case QtCriticalMsg:
      AppLogger::instance().critical(msg, category);
      break;
    case QtFatalMsg:
      AppLogger::instance().critical(msg, category);
      break;
    }

  inHandler = false;
}

} // end anonymous namespace

// Windows UTF-8 console support
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

Q_DECLARE_METATYPE(QPixmap)

namespace
{

//----------------------------------------------------------------------------
QPixmap createVisionMagicSplashPixmap()
{
  const QSize splashSize(640, 360);
  QPixmap splashPixmap(splashSize);
  splashPixmap.fill(Qt::transparent);

  QPainter painter(&splashPixmap);
  painter.setRenderHint(QPainter::Antialiasing, true);

  // Background gradient - professional blue theme
  QLinearGradient background(0.0, 0.0, 0.0, splashSize.height());
  background.setColorAt(0.0, QColor("#1a237e"));  // Deep indigo
  background.setColorAt(1.0, QColor("#0d1421"));  // Dark navy
  painter.fillRect(splashPixmap.rect(), background);

  // Accent overlay
  QLinearGradient accentOverlay(0.0, splashSize.height() * 0.25, splashSize.width(), splashSize.height());
  accentOverlay.setColorAt(0.0, QColor(255, 255, 255, 20));
  accentOverlay.setColorAt(1.0, QColor(33, 150, 243, 100));
  painter.setBrush(accentOverlay);
  painter.setPen(Qt::NoPen);
  const QRectF accentRect(40.0, splashSize.height() * 0.30, splashSize.width() - 80.0, splashSize.height() * 0.50);
  painter.drawRoundedRect(accentRect, 24.0, 24.0);

  // Main title - Chinese (use Microsoft YaHei for better Chinese rendering)
  painter.setPen(QColor("#ffffff"));
  QFont titleFont("Microsoft YaHei", 28, QFont::Bold);
  painter.setFont(titleFont);
  const QRectF titleRect(60.0, splashSize.height() * 0.18, splashSize.width() - 120.0, 50.0);
  painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                   QString::fromUtf8("\xe5\x8c\xbb\xe5\xad\xa6\xe5\xbd\xb1\xe5\x83\x8f\xe4\xb8\x89\xe7\xbb\xb4\xe9\x87\x8d\xe5\xbb\xba\xe8\xbd\xaf\xe4\xbb\xb6"));  // 医学影像三维重建软件

  // Subtitle - English
  painter.setPen(QColor(255, 255, 255, 220));
  QFont subtitleFont("Segoe UI", 16, QFont::Normal);
  painter.setFont(subtitleFont);
  const QRectF subtitleRect(60.0, splashSize.height() * 0.18 + 55.0, splashSize.width() - 120.0, 30.0);
  painter.drawText(subtitleRect, Qt::AlignLeft | Qt::AlignVCenter,
                   QString("Vision Magic Ecosystem"));

  // Version info
  painter.setPen(QColor(255, 255, 255, 180));
  QFont versionFont("Segoe UI", 12, QFont::DemiBold);
  painter.setFont(versionFont);
  const QRectF versionRect(60.0, splashSize.height() - 85.0, splashSize.width() - 120.0, 24.0);
  painter.drawText(versionRect, Qt::AlignLeft | Qt::AlignVCenter,
                   QString("V1.0.0.3"));

  // Loading text
  painter.setPen(QColor(255, 255, 255, 140));
  QFont loadingFont("Microsoft YaHei", 11);
  painter.setFont(loadingFont);
  const QRectF loadingRect(60.0, splashSize.height() - 55.0, splashSize.width() - 120.0, 20.0);
  painter.drawText(loadingRect, Qt::AlignLeft | Qt::AlignVCenter,
                   QString::fromUtf8("\xe6\xad\xa3\xe5\x9c\xa8\xe5\x88\x9d\xe5\xa7\x8b\xe5\x8c\x96\xef\xbc\x8c\xe8\xaf\xb7\xe7\xa8\x8d\xe5\x80\x99..."));  // 正在初始化，请稍候...

  // Copyright
  painter.setPen(QColor(255, 255, 255, 100));
  QFont metaFont("Segoe UI", 9);
  painter.setFont(metaFont);
  const QRectF metaRect(60.0, splashSize.height() - 30.0, splashSize.width() - 120.0, 18.0);
  painter.drawText(metaRect, Qt::AlignLeft | Qt::AlignVCenter,
                   QString("Powered by 3D Slicer | BSD License"));

  painter.end();

  return splashPixmap;
}

//----------------------------------------------------------------------------
int SlicerAppMain(int argc, char* argv[])
{
  // Windows UTF-8 support: Set console code page and enable UTF-8 for file paths
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif

  typedef qRadianceAppMainWindow SlicerMainWindowType;

  qSlicerApplicationHelper::preInitializeApplication(argv[0], new qAppStyle);

  // Initialize file logger as early as possible
  AppLogger::instance().initialize();
  qInstallMessageHandler(radianceQtMessageHandler);

  qSlicerApplication app(argc, argv);
  if (app.returnCode() != -1)
    {
    return app.returnCode();
    }

  // 设置任务栏和系统显示的应用程序名称
  QCoreApplication::setApplicationName(QString::fromUtf8("VisionMagic"));
  QGuiApplication::setApplicationDisplayName(QString::fromUtf8("医学影像三维重建软件"));

  // 配置 DICOM 数据库路径到程序目录下，避免用户目录中文路径导致的编码问题
  {
    QSettings settings;
    // DICOM schema 版本通常为 "0.9.3" 之类，这里使用通配符匹配常见版本
    QStringList schemaVersions = {"0.9.3", "0.9.2", "0.9.1", "0.9.0"};
    QString appDir = QCoreApplication::applicationDirPath();
    QString dicomDbPath = appDir + "/DICOMDatabase";
    
    // 确保目录存在
    QDir().mkpath(dicomDbPath);
    
    for (const QString& version : schemaVersions)
    {
      QString settingsKey = QString("DatabaseDirectory_%1").arg(version);
      if (settings.value(settingsKey).toString().isEmpty())
      {
        settings.setValue(settingsKey, dicomDbPath);
      }
    }
  }

  // 默认配置通过资源文件 DefaultSettings.ini 提供（Styles/Style、Modules/HomeModule）。
  // 此处不额外覆写用户偏好，保持简洁设计。

  qRegisterMetaType<QPixmap>("QPixmap");
  QScopedPointer<SlicerMainWindowType> window;
  QPixmap splashPixmap = createVisionMagicSplashPixmap();
  app.setProperty("SlicerCustomSplashPixmap", QVariant::fromValue(splashPixmap));
  QScopedPointer<QSplashScreen> splashScreen;

  qSlicerApplicationHelper::postInitializeApplication<SlicerMainWindowType>(
        app, splashScreen, window);

  if (!window.isNull())
    {
    // 固定显示软件名称：医学影像三维重建软件（Vision Magic Ecosystem）
    QString windowTitle = QString::fromUtf8("医学影像三维重建软件（Vision Magic Ecosystem）");
    window->setWindowTitle(windowTitle);
    }

  const int rc = app.exec();
  AppLogger::instance().shutdown();
  return rc;
}

} // end of anonymous namespace

#include "qSlicerApplicationMainWrapper.cxx"


