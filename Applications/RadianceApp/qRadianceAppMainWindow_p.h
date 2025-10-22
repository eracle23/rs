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

// Slicer forward declarations
class qSlicerAbstractModule;
class qSlicerModuleFinderDialog;
class qSlicerModulesListView;

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

protected:
  /// Apply Radiance palette to default Slicer toolbars and their actions.
  void applyToolbarBranding();

  /// Return the accent color used across the Radiance shell.
  QColor brandAccentColor() const;

  /// Return a monochrome version of the source icon tinted with the Radiance accent color.
  QIcon createTintedIcon(const QIcon& source, const QColor& tint) const;

  /// Create a Radiance-themed icon for module actions.
  QIcon createModuleIcon(const QIcon& baseIcon, const QColor& accentColor) const;

  /// Update the module selector combo and nested menus with Radiance-themed icons.
  void brandModuleSelectorMenu(const QColor& accentColor);

  /// Generate Radiance-specific glyph used for the Module Finder action.
  QIcon createModuleFinderIcon(const QColor& accentColor) const;

  /// Apply branding to the module finder dialog if it is currently visible.
  void brandAnyVisibleModuleFinder(const QColor& accentColor);

  /// Apply branding to a specific module finder dialog instance.
  void brandModuleFinderDialog(qSlicerModuleFinderDialog* dialog, const QColor& accentColor);

  /// Apply branding to the module list view backing the finder dialog.
  void brandModulesListView(qSlicerModulesListView* listView, const QColor& accentColor);

  /// Ensure a single module's QAction and related UI elements use Radiance icons.
  QIcon brandModuleByName(const QString& moduleName, const QColor& accentColor);

  /// Refresh branding across all currently loaded modules.
  void brandAllModules(const QColor& accentColor);
};

#endif
