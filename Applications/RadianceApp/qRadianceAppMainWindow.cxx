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

// Qt includes
#include <QDesktopWidget>
#include <QLabel>
#include <QToolBar>
#include <QDesktopServices>
#include <QUrl>

// Slicer includes
#include "qSlicerApplication.h"
#include "qSlicerAboutDialog.h"
#include "qSlicerMainWindow_p.h"
#include "qSlicerModuleSelectorToolBar.h"
#include <qMRMLWidget.h>

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
  helpAboutSlicerAppAction->setText(qRadianceAppMainWindow::tr("About %1").arg(qSlicerApplication::application()->mainApplicationDisplayName()));
  QObject::connect(helpAboutSlicerAppAction, &QAction::triggered,
                   q, &qRadianceAppMainWindow::on_HelpAboutRadianceAppAction_triggered);

  //----------------------------------------------------------------------------
  // Calling "setupUi()" after adding the actions above allows the call
  // to "QMetaObject::connectSlotsByName()" done in "setupUi()" to
  // successfully connect each slot with its corresponding action.
  this->Superclass::setupUi(mainWindow);

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
  if (this->EditMenu)
    {
    this->EditMenu->menuAction()->setVisible(false);
    }
  if (this->ViewMenu)
    {
    this->ViewMenu->menuAction()->setVisible(false);
    }
  if (this->LayoutMenu)
    {
    this->LayoutMenu->menuAction()->setVisible(false);
    }
  if (this->WindowToolBarsMenu)
    {
    this->WindowToolBarsMenu->menuAction()->setVisible(false);
    }
  if (this->AppearanceMenu)
    {
    this->AppearanceMenu->menuAction()->setVisible(false);
    }

  //----------------------------------------------------------------------------
  // Configure
  //----------------------------------------------------------------------------
  mainWindow->setWindowIcon(QIcon(":/Icons/Medium/DesktopIcon.png"));

  QLabel* logoLabel = new QLabel();
  logoLabel->setObjectName("LogoLabel");
  logoLabel->setPixmap(qMRMLWidget::pixmapFromIcon(QIcon(":/LogoFull.png")));
  this->PanelDockWidget->setTitleBarWidget(logoLabel);
  this->PanelDockWidget->setWindowTitle(qRadianceAppMainWindow::tr("Workflow"));

  auto hideToolbar = [](QToolBar* toolbar)
    {
      if (!toolbar)
        {
        return;
        }
      toolbar->setVisible(false);
      toolbar->setEnabled(false);
    };

  hideToolbar(this->ModuleSelectorToolBar);
  hideToolbar(this->ModuleToolBar);
  hideToolbar(this->UndoRedoToolBar);
  hideToolbar(this->ViewToolBar);
  hideToolbar(this->ViewersToolBar);
  hideToolbar(this->MouseModeToolBar);
  hideToolbar(this->DialogToolBar);
  hideToolbar(this->LayoutToolBar);

  if (this->DataProbeCollapsibleWidget)
    {
    this->DataProbeCollapsibleWidget->setVisible(false);
    }

  if (this->MainToolBar)
    {
    this->MainToolBar->setWindowTitle(qRadianceAppMainWindow::tr("Data I/O"));
    }

  // Hide the menus
  //this->menubar->setVisible(false);
  //this->FileMenu->setVisible(false);
  //this->EditMenu->setVisible(false);
  //this->ViewMenu->setVisible(false);
  //this->LayoutMenu->setVisible(false);
  //this->HelpMenu->setVisible(false);
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
  qSlicerAboutDialog about(this);
  about.setLogo(QPixmap(":/Logo.png"));
  about.exec();
}
