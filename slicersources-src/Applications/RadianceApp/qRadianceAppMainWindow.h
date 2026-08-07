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

#ifndef __qRadianceAppMainWindow_h
#define __qRadianceAppMainWindow_h

// Radiance includes
#include "qRadianceAppExport.h"
class qRadianceAppMainWindowPrivate;

// Slicer includes
#include "qSlicerMainWindow.h"

class Q_RADIANCE_APP_EXPORT qRadianceAppMainWindow : public qSlicerMainWindow
{
  Q_OBJECT
public:
  typedef qSlicerMainWindow Superclass;

  qRadianceAppMainWindow(QWidget *parent=0);
  virtual ~qRadianceAppMainWindow();

public slots:
  void on_HelpAboutRadianceAppAction_triggered();
  /// 保存（弹窗提示成功/失败）
  void on_FileSaveSceneAction_triggered() override;
  void on_SDBSaveToDirectoryAction_triggered() override;
  void on_SDBSaveToMRBAction_triggered() override;
  // Keep access consistent with base class (qSlicerMainWindow declares this in public slots)
  void setHomeModuleCurrent() override;
  void on_FavoriteModulesChanged() override;

protected slots:
  void onModuleLoaded(const QString& moduleName) override;

protected:
  qRadianceAppMainWindow(qRadianceAppMainWindowPrivate* pimpl, QWidget* parent);

private:
  Q_DECLARE_PRIVATE(qRadianceAppMainWindow);
  Q_DISABLE_COPY(qRadianceAppMainWindow);

  void applyShellTweaks();
};

#endif
