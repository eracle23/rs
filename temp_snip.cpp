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
  helpAboutSlicerAppAction->setText(qRadianceAppMainWindow::tr("About %1").arg(qSlicerApplication::application()->applicationName()));
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
