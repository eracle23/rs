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
// Theme/Settings
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>

Q_DECLARE_METATYPE(QPixmap)

namespace
{

//----------------------------------------------------------------------------
QPixmap createAliceSplashPixmap()
{
  const QSize splashSize(640, 360);
  QPixmap splashPixmap(splashSize);
  splashPixmap.fill(Qt::transparent);

  QPainter painter(&splashPixmap);
  painter.setRenderHint(QPainter::Antialiasing, true);

  QLinearGradient background(0.0, 0.0, 0.0, splashSize.height());
  background.setColorAt(0.0, QColor("#5a6cff"));
  background.setColorAt(1.0, QColor("#1c2443"));
  painter.fillRect(splashPixmap.rect(), background);

  QLinearGradient accentOverlay(0.0, splashSize.height() * 0.25, splashSize.width(), splashSize.height());
  accentOverlay.setColorAt(0.0, QColor(255, 255, 255, 24));
  accentOverlay.setColorAt(1.0, QColor(84, 104, 255, 128));
  painter.setBrush(accentOverlay);
  painter.setPen(Qt::NoPen);
  const QRectF accentRect(40.0, splashSize.height() * 0.35, splashSize.width() - 80.0, splashSize.height() * 0.45);
  painter.drawRoundedRect(accentRect, 42.0, 42.0);

  painter.setPen(QColor("#ffffff"));
  QFont titleFont("Segoe UI", 36, QFont::Bold);
  painter.setFont(titleFont);
  const QRectF titleRect(60.0, splashSize.height() * 0.24, splashSize.width() - 120.0, 60.0);
  painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, QObject::tr("Alice Studio"));

  painter.setPen(QColor(255, 255, 255, 200));
  QFont taglineFont("Segoe UI", 14);
  painter.setFont(taglineFont);
  const QRectF taglineRect(60.0, splashSize.height() * 0.24 + 70.0, splashSize.width() - 120.0, 40.0);
  painter.drawText(taglineRect, Qt::AlignLeft | Qt::AlignVCenter, QObject::tr("Curated imaging workspace built on Slicer technology"));

  painter.setPen(QColor(255, 255, 255, 160));
  QFont versionFont("Segoe UI", 12, QFont::DemiBold);
  painter.setFont(versionFont);
  const QRectF versionRect(60.0, splashSize.height() - 80.0, splashSize.width() - 120.0, 24.0);
  painter.drawText(versionRect, Qt::AlignLeft | Qt::AlignVCenter,
                   QObject::tr("Initializing please wait..."));

  painter.setPen(QColor(255, 255, 255, 120));
  QFont metaFont("Segoe UI", 10);
  painter.setFont(metaFont);
  const QRectF metaRect(60.0, splashSize.height() - 48.0, splashSize.width() - 120.0, 20.0);
  painter.drawText(metaRect, Qt::AlignLeft | Qt::AlignVCenter,
                   QObject::tr("Alice Labs - Powered by 3D Slicer under BSD license"));

  painter.end();

  return splashPixmap;
}

//----------------------------------------------------------------------------
int SlicerAppMain(int argc, char* argv[])
{
  typedef qRadianceAppMainWindow SlicerMainWindowType;

  qSlicerApplicationHelper::preInitializeApplication(argv[0], new qAppStyle);

  qSlicerApplication app(argc, argv);
  if (app.returnCode() != -1)
    {
    return app.returnCode();
    }

  // 设置任务栏和系统显示的应用程序名称
  QCoreApplication::setApplicationName(QString::fromUtf8("VisionMagic"));
  QGuiApplication::setApplicationDisplayName(QString::fromUtf8("医学影像三维重建软件"));

  // 默认配置通过资源文件 DefaultSettings.ini 提供（Styles/Style、Modules/HomeModule）。
  // 此处不额外覆写用户偏好，保持简洁设计。

  qRegisterMetaType<QPixmap>("QPixmap");
  QScopedPointer<SlicerMainWindowType> window;
  QPixmap splashPixmap = createAliceSplashPixmap();
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

  return app.exec();
}

} // end of anonymous namespace

#include "qSlicerApplicationMainWrapper.cxx"


