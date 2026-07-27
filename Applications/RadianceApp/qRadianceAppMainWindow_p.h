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

#ifndef __qRadianceAppMainWindow_p_h
#define __qRadianceAppMainWindow_p_h

// Radiance includes
#include "qRadianceAppMainWindow.h"

// Slicer includes
#include "qSlicerMainWindow_p.h"

// Qt forward declarations
class QAction;
class QIcon;
class QColor;
class ThemeSync;
class SliceColorAdapter;
class SystemColorSchemeWatcher;

// Slicer forward declarations
class qSlicerAbstractModule;

//-----------------------------------------------------------------------------
class Q_RADIANCE_APP_EXPORT qRadianceAppMainWindowPrivate
  : public qSlicerMainWindowPrivate
{
  Q_DECLARE_PUBLIC(qRadianceAppMainWindow);
public:
  typedef qSlicerMainWindowPrivate Superclass;
  qRadianceAppMainWindowPrivate(qRadianceAppMainWindow& object);
  virtual ~qRadianceAppMainWindowPrivate();

  virtual void init();
  /// Reimplemented for custom behavior
  virtual void setupUi(QMainWindow * mainWindow);
  void applyThemeMode(const QString& mode, bool persist);
  void applyBrandHeaderTheme(QMainWindow* mainWindow, const QString& mode);

protected:
  QString CurrentThemeMode{QStringLiteral("dark")};
  QColor brandAccentColor() const;
  QIcon createTintedIcon(const QIcon& source, const QColor& tint) const;
  QIcon createModuleIcon(const QIcon& baseIcon, const QColor& accentColor) const;
  QIcon createModuleFinderIcon(const QColor& accentColor) const;
  QIcon brandModuleByName(const QString& moduleName, const QColor& accentColor);

  ThemeSync* ThemeSyncHandler{nullptr};
  SliceColorAdapter* SliceColorAdapterHandler{nullptr};
  SystemColorSchemeWatcher* SystemColorSchemeWatcherHandler{nullptr};
};

#endif
