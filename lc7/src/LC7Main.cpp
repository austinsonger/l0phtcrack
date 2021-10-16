#include "stdafx.h"
#include "LC7Main.h"

QIcon readIcoFile(const QString & path)
{
  QImageReader ir(path);
  QIcon icon;
 
  if (ir.canRead())
  {
    do
    {
      icon.addPixmap(QPixmap::fromImage(ir.read()));
    }
    while (ir.jumpToNextImage());
  }
 
  return icon;
}


LC7Main::LC7Main(ILC7Controller *ctrl, QWidget *parent) : QMainWindow(parent, 
	Qt::WindowTitleHint |
	Qt::WindowSystemMenuHint |
	Qt::WindowMinMaxButtonsHint |
	Qt::WindowCloseButtonHint |
	Qt::WindowFullscreenButtonHint), m_updater(ctrl)
{	
	ui.setupUi(this);
	
	m_ctrl=ctrl;
	
	QIcon ico=readIcoFile(":/lc7/lc7.ico");
	setWindowIcon(ico);

	m_pStartupWidget = NULL;
	m_batch_workqueue=NULL;
	m_single_workqueue=NULL;
	m_pColorManager = NULL;
	m_shade = NULL;
	m_shadecount = 0;

	m_pWorkQueueWidget = NULL;

	connect(ui.menuButton,SIGNAL(pressed()),this,SLOT(onMenuPressed()));
	
//	ui.menuButton->setObjectName("menuButton");
//	ui.helpButton->setObjectName("helpButton");

	m_menu = new QMenu(this);
	m_action_new_session=m_menu->addAction("&New Session",this,SLOT(onNewSession()),QKeySequence("Ctrl+N"));
	m_action_open_session=m_menu->addAction("&Open Session...",this,SLOT(onOpenSession()),QKeySequence("Ctrl+O"));
	m_menu_recent_sessions = m_menu->addMenu("&Recent Sessions");
	m_action_restore_session = m_menu->addAction("&Recover Autosaved Session...", this, SLOT(slot_restoreAutosaveSession()));
	m_action_save_session = m_menu->addAction("&Save Session", this, SLOT(onSaveSession()), QKeySequence("Ctrl+S"));
	m_action_save_session_as=m_menu->addAction("Save Session &As...",this,SLOT(onSaveSessionAs()));
	m_action_close_session=m_menu->addAction("&Close Session",this,SLOT(onCloseSession()));

	m_menu->addSeparator();

	m_action_custom_sep = m_menu->addSeparator();

#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	m_action_fullscreen=m_menu->addAction("&Fullscreen", this, SLOT(onFullscreen()));
	m_action_fullscreen->setCheckable(true);
	m_action_fullscreen->setChecked(false);
#endif

	m_action_systray = m_menu->addAction("Minimize To System &Tray", this, SLOT(onMinimizeToSystemTray()));
#ifndef DISABLE_UPDATER
	m_action_check_for_updates = m_menu->addAction("Check For &Updates", this, SLOT(onCheckForUpdates()));
#endif

	m_menu->addSeparator();

	m_action_quit=m_menu->addAction("&Quit",this,SLOT(onQuit()),QKeySequence("Ctrl+Q"));

	connect(&m_trayicon, &QSystemTrayIcon::activated, this, &LC7Main::slot_systemTray_activated);
	connect(&m_trayicon, &QSystemTrayIcon::messageClicked, this, &LC7Main::slot_systemTray_messageClicked);

#ifndef DISABLE_UPDATER
	connect(&m_updater, &CLC7Updater::sig_doUpdate, this, &LC7Main::slot_doUpdate);
#endif

	m_trayicon.setIcon(qApp->windowIcon());

}


LC7Main::~LC7Main()
{TR;

}

void LC7Main::slot_helpButtonClicked()
{TR;
	m_ctrl->GetSettings()->setValue("_ui_:enable_help",ui.helpButton->isChecked());
}


void LC7Main::closeEvent (QCloseEvent *evt)
{TR;
	if(m_ctrl->IsAnyQueueRunning())
	{
		m_ctrl->GetGUILinkage()->ErrorMessage("Queue is running", "An operation is in progress. Stop the queue or pause it before performing this action.");
		evt->ignore();
		return;
	}

	QString oldsessionfile;
	if(m_ctrl->IsSessionOpen(&oldsessionfile))
	{
		if(!SaveIfModified("Session has been modified, closing this session will lose your changes."))
		{
			UpdateUI();
			evt->ignore();
    		return;
		}
		if(!m_ctrl->CloseSession())
		{
			m_ctrl->GetGUILinkage()->ErrorMessage("Couldn't close session", "Session close failed. Contact support@l0phtcrack.com.");
			UpdateUI();
		    evt->ignore();
    		return;
		}
	}

	evt->accept();
}

void LC7Main::Startup()
{	
	m_pColorManager = new CLC7ColorManager();
	
#ifdef _DEBUG
	QShortcut *reload_shortcut = new QShortcut(QKeySequence("Ctrl+R"), this);
	QObject::connect(reload_shortcut, SIGNAL(activated()), m_pColorManager, SLOT(reload()));
#endif

	// Create navbar
	m_pNavBar = new CLC7NavBar(ui.stackWidget);
	delete ui.navbar;
	ui.tabLayout->insertWidget(0, m_pNavBar);
	ui.navbar = m_pNavBar;
	ui.navbar->setGraphicsEffect(m_ctrl->GetGUILinkage()->GetColorManager()->CreateShadowEffect());

	// Hook up gui to controller notifications
	m_ctrl->RegisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&LC7Main::NotifySessionActivity);
	m_ctrl->RegisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&LC7Main::NotifySessionActivity);

	// Set help button state and connect it
	ui.helpButton->setChecked(m_ctrl->GetSettings()->value("_ui_:enable_help",true).toBool());
	connect(ui.helpButton,&QAbstractButton::clicked,this, &LC7Main::slot_helpButtonClicked);

	// Try to get splitter to look right
	QList<int> sizes;
	sizes << 1000;
	sizes << 200;
	ui.splitter->setSizes(sizes);

	// Set up workqueue widget
	m_pWorkQueueWidget = new CLC7WorkQueueWidget(ui.splitter, m_ctrl);
	delete ui.workQueueWidget;
	ui.splitter->addWidget(m_pWorkQueueWidget);
	ui.workQueueWidget = m_pWorkQueueWidget;
	m_pWorkQueueWidget->Decorate();

	// Set up update timer
	m_update_ui_later_timer.setInterval(1000);
	m_update_ui_later_timer.start();
	connect(&m_update_ui_later_timer, &QTimer::timeout, this, &LC7Main::slot_update_ui_later);

	// Set up recolor callback and run it once
	m_pColorManager->RegisterRecolorCallback(this, (void (QObject::*)())&LC7Main::slot_recolorCallback);
	m_pColorManager->ReloadSettings();

	UpdateUI();
}

void LC7Main::Shutdown(void)
{
	m_update_ui_later_timer.stop();

	// Shut down workqueue widget threads
	if (m_pWorkQueueWidget)
	{
		m_pWorkQueueWidget->Shutdown();
	}

	m_ctrl->UnregisterNotifySessionActivity(SINGLE_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&LC7Main::NotifySessionActivity);
	m_ctrl->UnregisterNotifySessionActivity(BATCH_WORKQUEUE_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&LC7Main::NotifySessionActivity);

	if (m_pColorManager)
	{
		delete m_pColorManager;
		m_pColorManager = NULL;
	}
}

void LC7Main::slot_recolorCallback(void)
{TR;
	// Resize menu bar and buttons based on dpi
	int ratio=m_pColorManager->GetSizeRatio();

	ui.menuButton->setFixedSize(67 * ratio, 26 * ratio);
	ui.helpButton->setFixedSize(67 * ratio, 26 * ratio);
	ui.menuBarWidget->setFixedHeight(32 * ratio);
	ui.navbar->setFixedWidth(160 * ratio);
}


void LC7Main::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{TR;
	switch(activity)
	{
	case ILC7Linkage::SESSION_OPEN_POST:
	case ILC7Linkage::SESSION_NEW_POST:
		if (handler && handler->GetId() == BATCH_WORKQUEUE_HANDLER_ID)
		{
			m_batch_workqueue = (ILC7WorkQueue *)handler;
			
			for (int i = 0; i < m_main_tab_widgets.count(); i++)
			{
				QWidget *pMainMenuTab = m_main_tab_widgets[i];
				if (m_batch_workqueue)
				{
					m_batch_workqueue->RegisterUIEnable(pMainMenuTab);
				}
			}
		}
		if (handler && handler->GetId() == SINGLE_WORKQUEUE_HANDLER_ID)
		{
			m_single_workqueue = (ILC7WorkQueue *)handler;

			for (int i = 0; i<m_main_tab_widgets.count(); i++)
			{
				QWidget *pMainMenuTab = m_main_tab_widgets[i];
				if (m_single_workqueue)
				{
					m_single_workqueue->RegisterUIEnable(pMainMenuTab);
				}
			}
		}


		break;
	case ILC7Linkage::SESSION_CLOSE_PRE:
		if (handler && handler->GetId() == BATCH_WORKQUEUE_HANDLER_ID)
		{
			m_batch_workqueue = NULL;		
		}
		if (handler && handler->GetId() == SINGLE_WORKQUEUE_HANDLER_ID)
		{
			m_single_workqueue = NULL;		
		}
		break;
	}
}

bool LC7Main::SwitchToMainMenuTab(QString strInternalName)
{TR;
	QWidget *widget=m_main_tab_widgets_by_internal_name.value(strInternalName,NULL);
	if(!widget)
	{
		return false;
	}

	CLC7NavBarItem *item=m_pNavBar->item(widget);
	if(!item)
	{
		return false;
	}

	item->setChecked(true);
	ui.stackWidget->setCurrentWidget(widget);

	return true;
}

bool LC7Main::AddMainMenuTab(QString strTabTitle, QString strInternalName, QWidget *pMainMenuTab)
{TR;
	m_main_tab_widgets.append(pMainMenuTab);
	m_main_tab_widgets_by_internal_name.insert(strInternalName, pMainMenuTab);

	ui.stackWidget->addWidget(pMainMenuTab);

	QString groupname="Base";
	if(strTabTitle.contains("/"))
	{
		QStringList titleparts=strTabTitle.split("/");
		groupname=titleparts[0];
		strTabTitle=titleparts[1];
	}

//	bool default_tab=false;
//	if(strTabTitle.endsWith("*"))
//	{
//		default_tab=true;
//		strTabTitle=strTabTitle.left(strTabTitle.length()-1);
//	}
	
	CLC7NavBarGroup *group=m_pNavBar->group(groupname);
	if(!group)
	{
		group=m_pNavBar->insertGroup(0,groupname);
		group->setTitle(groupname);
	}
	CLC7NavBarItem *item=group->insertItem(0,pMainMenuTab);
	item->setText(strTabTitle);

//	if(default_tab)
//	{
//		item->setChecked(true);
//		ui.stackWidget->setCurrentWidget(pMainMenuTab);
//	}

	if(m_batch_workqueue)
	{
		m_batch_workqueue->RegisterUIEnable(pMainMenuTab);
	}
	if(m_single_workqueue)
	{
		m_single_workqueue->RegisterUIEnable(pMainMenuTab);
	}

	return true;
}

bool LC7Main::RemoveMainMenuTab(QWidget *pMainMenuTab)
{TR;
	int idx=m_main_tab_widgets.indexOf(pMainMenuTab);
	if(idx==-1)
	{
		return false;
	}

 	m_main_tab_widgets.removeAt(idx);
	
	CLC7NavBarItem *item=m_pNavBar->item(pMainMenuTab);
	CLC7NavBarGroup *group=item->group();
	group->removeItem(item);
	if(group->count()==0)
	{
		m_pNavBar->removeGroup(group);
	}

	ui.stackWidget->removeWidget(pMainMenuTab);

	return true;
}

QWidget *LC7Main::GetMainMenuTab(QString strInternalName)
{
	TR;
	return m_main_tab_widgets_by_internal_name[strInternalName];
}

void LC7Main::SwitchToFirstTab()
{TR;
	CLC7NavBarGroup *group=m_pNavBar->group(0);
	if(group)
	{
		CLC7NavBarItem *item=group->item(0);
		if(item)
		{
			item->setChecked(true);
			ui.stackWidget->setCurrentWidget(item->buddy());
		}
	}
}

void LC7Main::AddTopMenuItem(ILC7Action *act)
{
	QAction *qact = new QAction(act->Icon(), act->Name(), 0);
	qact->setToolTip(act->Desc());
	qact->setStatusTip(act->Desc());
	m_menu->insertAction(m_action_custom_sep, qact);
	m_custom_top_menu_actions.insert(act, qact);

	connect(qact, &QAction::triggered, this, &LC7Main::onCustomAction);
}

void LC7Main::RemoveTopMenuItem(ILC7Action *act)
{
	QAction *qact = m_custom_top_menu_actions[act];
	if (qact == NULL)
	{
		return;
	}
	m_menu->removeAction(qact);
	m_custom_top_menu_actions.remove(act);
	delete qact;
}

void LC7Main::AddStartupDialogItem(ILC7Action *act)
{
	m_custom_startup_dialog_actions.append(act);
}

void LC7Main::RemoveStartupDialogItem(ILC7Action *act)
{
	m_custom_startup_dialog_actions.removeOne(act);
}



QAbstractButton *LC7Main::GetHelpButton(void)
{TR;
	return ui.helpButton;
}

void LC7Main::slot_update_ui_later(void)
{
	if (m_update_ui_later)
	{
		m_update_ui_later = false;

		UpdateUI();
	}
}

void LC7Main::showEvent(QShowEvent* event) 
{
	QWidget::showEvent(event);

	connect(windowHandle(), &QWindow::screenChanged, m_pColorManager, &CLC7ColorManager::slot_screenChanged);
}

void LC7Main::UpdateUILater()
{
	m_update_ui_later = true;
}

void LC7Main::UpdateUI()
{
	QString windowTitle = QString("L0phtCrack 7 - v%1").arg(VERSION_STRING);
	
	QString sessionfile;
	bool is_open=false;
	bool is_modified=false;
	if(m_ctrl->IsSessionOpen(&sessionfile))
	{
		is_open=true;
		if(sessionfile=="")
		{
			windowTitle += QString(" [Unnamed Session]");
		}
		else
		{
			windowTitle += QString(" [%1]").arg(sessionfile);		
		}
		if(m_ctrl->IsSessionModified())
		{
			is_modified=true;
			windowTitle += " *";
		}
	}
	else
	{
		windowTitle += QString(" [No Session]");
	}

	setWindowTitle(windowTitle);

	QWidget *widget = qApp->activeModalWidget();
	if (!is_open && (!m_pStartupWidget || !m_pStartupWidget->isVisible()) && widget==NULL)
	{
		ui.NoSessionLabel->setVisible(true);
		ui.NoSessionInnerLabel->setObjectName("warning");
	}
	else
	{
		ui.NoSessionLabel->setVisible(false);
	}

	m_action_new_session->setEnabled(true);
	m_action_open_session->setEnabled(true);
	m_action_restore_session->setEnabled(m_ctrl->IsAutoSaveAvailable());
	m_action_save_session->setEnabled(is_modified);
	m_action_save_session_as->setEnabled(is_open);
	m_action_close_session->setEnabled(is_open);
	m_action_quit->setEnabled(true);
#ifndef DISABLE_UPDATER
	m_action_check_for_updates->setEnabled(!m_updater.isRunning());
#endif
	m_action_fullscreen->setChecked(windowState() & Qt::WindowFullScreen);

	m_menu_recent_sessions->setEnabled(m_menu_recent_sessions->actions().size() != 0);
}

CLC7WorkQueueWidget *LC7Main::GetWorkQueueWidget()
{
	return m_pWorkQueueWidget;
}

void LC7Main::updateRecentSessionsMenu()
{
	m_menu_recent_sessions->clear();

	QStringList recentsessionfiles = CLC7App::getInstance()->GetController()->GetRecentSessionsList();
	foreach(QString recentsessionfile, recentsessionfiles)
	{
		QAction *act = m_menu_recent_sessions->addAction(QFileInfo(recentsessionfile).fileName());
		connect(act, &QAction::triggered, this, &LC7Main::slot_recentSessionsMenu);
		act->setData(recentsessionfile);
	}
}

void LC7Main::slot_recentSessionsMenu()
{
	QAction *act = (QAction *)sender();

	QString recentsession = act->data().toString();

	DoOpenSession(recentsession);
}


void LC7Main::onMenuPressed()
{TR;
	updateRecentSessionsMenu();
	UpdateUI();
	m_menu->popup(ui.menuButton->mapToGlobal(QPoint(0,26)));
}

void LC7Main::onNewSession()
{TR;
	DoNewSession();
}

bool LC7Main::DoNewSession(void)
{
	TR;
	if (m_ctrl->IsAnyQueueRunning())
	{
		m_ctrl->GetGUILinkage()->ErrorMessage("Queue is running", "An operation is in progress. Stop the queue or pause it before performing this action.");
		return false;
	}

	QString sessionfile;
	if (m_ctrl->IsSessionOpen(&sessionfile))
	{
		if (!SaveIfModified("In order to create a new session, the current one must be closed."))
		{
			UpdateUI();
			return false;
		}
		if (!m_ctrl->CloseSession())
		{
			m_ctrl->GetGUILinkage()->ErrorMessage("Couldn't close session", "Session close failed. Contact support@l0phtcrack.com.");
			UpdateUI();
			return false;
		}
	}
	if (!m_ctrl->NewSession())
	{
		m_ctrl->GetGUILinkage()->ErrorMessage("Couldn't create session", "Session creation failed. Contact support@l0phtcrack.com.");
		UpdateUI();
		return false;
	}
	ShowStartupDialog(false);
	UpdateUI();
	return true;
}


bool LC7Main::DoOpenSession(QString requested_session)
{TR;
	if (m_ctrl->IsAnyQueueRunning())
	{
		m_ctrl->GetGUILinkage()->ErrorMessage("Queue is running", "An operation is in progress. Stop the queue or pause it before performing this action.");
		return false;
	}

	QString oldsessionfile;
	if (m_ctrl->IsSessionOpen(&oldsessionfile))
	{
		if (!SaveIfModified("In order to open another session, the current one must be closed."))
		{
			UpdateUI();
			return false;
		}
		if (!m_ctrl->CloseSession())
		{
			m_ctrl->GetGUILinkage()->ErrorMessage("Couldn't close session", "Session close failed. Contact support@l0phtcrack.com.");
			UpdateUI();
			return false;
		}
	}

	
	QString sessiondir = m_ctrl->GetSessionsDirectory();
	QString sessionfile;
	if (!requested_session.isNull())
	{
		sessionfile = requested_session;
	}
	else
	{ 
		if (!m_ctrl->GetGUILinkage()->OpenFileDialog("Open Session...", sessiondir, "LC7 Session Files (*.lc7)", sessionfile))
		{
			UpdateUI();
			return false;
		}
	}
	if (sessionfile.isEmpty())
	{
		UpdateUI();
		return false;
	}
	if (!m_ctrl->OpenSession(sessionfile))
	{
		m_ctrl->GetGUILinkage()->ErrorMessage("Couldn't open session", 
			"Opening session file failed. You may not have permission to open the file, or the file is corrupted. If the file is in use in another application is must first be closed.");
		UpdateUI();
		return false;
	}
	else
	{
		GetWorkQueueWidget()->AppendToActivityLog("Session loaded.\n");
		GetWorkQueueWidget()->ScrollToBottom();
	}

	ShowStartupDialog(false);
	UpdateUI();
	return true;
}

bool LC7Main::DoPauseBackgroundSession(QString sessionFile)
{TR;
	if (!PauseOrStopAllQueues(false))
	{
		m_ctrl->GetGUILinkage()->ErrorMessage("Couldn't stop session", "Session pause or stop failed. Contact support@l0phtcrack.com.");
		UpdateUI();
		return false;
	}

	if (!m_ctrl->SaveSession(sessionFile))
	{
		QCoreApplication::exit();
		return false;
	}

	if (!m_ctrl->CloseSession())
	{
		QCoreApplication::exit();
		return false;
	}

	close();
	return true;
}

void LC7Main::onOpenSession()
{TR;
	DoOpenSession(QString());
}

void LC7Main::onSaveSession()
{TR;
	if(m_ctrl->IsAnyQueueRunning())
	{
		m_ctrl->GetGUILinkage()->ErrorMessage("Queue is running", "An operation is in progress. Stop the queue or pause it before performing this action.");
		return;
	}

	SaveSessionOrSaveAsSession();
	UpdateUI();
}

void LC7Main::onSaveSessionAs()
{TR;
	if(m_ctrl->IsAnyQueueRunning())
	{
		m_ctrl->GetGUILinkage()->ErrorMessage("Queue is running", "An operation is in progress. Stop the queue or pause it before performing this action.");
		return;
	}

	QString sessionfile;
	if(!m_ctrl->IsSessionOpen(&sessionfile))
	{
		return;
	}

	QString sessiondir = m_ctrl->GetSessionsDirectory();
	if (!m_ctrl->GetGUILinkage()->SaveFileDialog("Save Session As...", sessiondir, "LC7 Session Files (*.lc7)", sessionfile))
	{
		return;
	}
	if(!m_ctrl->SaveSession(sessionfile))
	{
		m_ctrl->GetGUILinkage()->ErrorMessage("Couldn't save session", "Session save failed. You may not have permission to save the session here. If the target file is in use, the application using it must first be closed.");
	}
	else
	{
		GetWorkQueueWidget()->AppendToActivityLog("Session saved.\n");
	}
}

bool LC7Main::PauseOrStopAllQueues(bool allow_stop)
{TR;
	if (m_batch_workqueue && m_batch_workqueue->GetWorkQueueState() == ILC7WorkQueue::IN_PROGRESS)
	{
		bool stopped = false;
		if (m_batch_workqueue->IsPauseEnabled())
		{
			if (m_batch_workqueue->PauseRequest())
			{
				stopped = true;
			}
		}
		if (allow_stop)
		{
			if (!stopped && m_batch_workqueue->IsStopEnabled())
			{
				if (m_batch_workqueue->StopRequest())
				{
					stopped = true;
				}
			}
		}
		if(!stopped)
		{
			// Can't stop!
			return false;
		}

		// Wait for queue to stop
		while (m_batch_workqueue->GetWorkQueueState() == ILC7WorkQueue::IN_PROGRESS)
		{
			QCoreApplication::processEvents();
			QThread::yieldCurrentThread();
		}
	}
	else if (m_single_workqueue && m_single_workqueue->GetWorkQueueState() == ILC7WorkQueue::IN_PROGRESS)
	{
		bool stopped = false;
		if (m_single_workqueue->IsPauseEnabled())
		{
			if (m_single_workqueue->PauseRequest())
			{
				stopped = true;
			}
		}
		if (allow_stop)
		{
			if (!stopped && m_single_workqueue->IsStopEnabled())
			{
				if (m_single_workqueue->StopRequest())
				{
					stopped = true;
				}
			}
		}
		if (!stopped)
		{
			// Can't stop!
			return false;
		}

		// Wait for queue to stop
		while (m_single_workqueue->GetWorkQueueState() == ILC7WorkQueue::IN_PROGRESS)
		{
			QCoreApplication::processEvents();
			QThread::yieldCurrentThread();
		}
	}

	// No queues or already stopped
	return true;
}

bool LC7Main::DoCloseSession(bool force, bool allow_startup_dialog)
{TR;
	QString oldsessionfile;
	if (!m_ctrl->IsSessionOpen(&oldsessionfile))
	{
		return true;
	}

	if (!force)
	{
		if (Attach::isInteractiveSession() &&
			m_ctrl->IsAnyQueueRunning() && !m_ctrl->GetGUILinkage()->YesNoBox("Must stop queue operation", "To proceed you must stop the currently executing operation. Would you like to proceed?"))
		{
			return false;
		}
	}

	if (!PauseOrStopAllQueues(true))
	{
		m_ctrl->GetGUILinkage()->ErrorMessage("Couldn't stop session", "Session pause or stop failed. Contact support@l0phtcrack.com.");
		UpdateUI();
		return false;
	}
	
	if (!force)
	{
		if (Attach::isInteractiveSession() &&
			!SaveIfModified("Session has been modified, closing this session will lose your changes."))
		{
			UpdateUI();
			return false;
		}
	}
	if (!m_ctrl->CloseSession())
	{
		if (Attach::isInteractiveSession())
		{
			m_ctrl->GetGUILinkage()->ErrorMessage("Couldn't close session", "Session close failed. Contact support@l0phtcrack.com.");
		}
		UpdateUI();
		return false;
	}

	if (allow_startup_dialog)
	{
		ILC7Settings *settings = m_ctrl->GetSettings();
		bool show_startup_dialog = settings->value("_ui_:showstartupdialog", true).toBool();
		if (show_startup_dialog)
		{
			ShowStartupDialog(true);
		}
	}

	UpdateUI();
	return true;
}

void LC7Main::onCustomAction()
{
	if (m_ctrl->IsAnyQueueRunning())
	{
		m_ctrl->GetGUILinkage()->ErrorMessage("Queue is running", "An operation is in progress. Stop the queue or pause it before performing this action.");
		return;
	}

	QAction *qact = (QAction *)sender();

	ILC7Action *act = m_custom_top_menu_actions.key(qact);
	if (act)
	{
		ILC7Component *comp = m_ctrl->FindComponentByID(act->ComponentId());
		QMap<QString, QVariant> config;
		QString error;
		if (comp->ExecuteCommand(act->Command(), act->Args(), config, error) != ILC7Component::SUCCESS)
		{
			m_ctrl->GetGUILinkage()->ErrorMessage("Error running wizard", error);
			return;
		}
	}
}

void LC7Main::onCloseSession()
{TR;
	DoCloseSession(false, true);
}
	
void LC7Main::onFullscreen()
{TR;
	if(windowState() & Qt::WindowFullScreen)
	{
		setWindowState(windowState() & ~Qt::WindowFullScreen);
	}
	else
	{
		setWindowState(windowState() | Qt::WindowFullScreen);
	}
}


void LC7Main::onMinimizeToSystemTray()
{TR;
	if (!m_trayicon.isVisible())
	{
		DoMinimizeToSystemTray();
	}
}

void LC7Main::onCheckForUpdates()
{TR;
	DoCheckForUpdates(true);
}

void LC7Main::onQuit()
{TR;
	close();
}

void LC7Main::resizeEvent(QResizeEvent * evt)
{TR;
	QSize winsz = size();
	if (m_pStartupWidget)
	{
		m_pStartupWidget->move(0,0);
		m_pStartupWidget->resize(winsz);
	}
	if (m_shade)
	{
		m_shade->move(0, 0);
		m_shade->resize(winsz);
	}
	UpdateUI();
}


QString LC7Main::GetRecentSessionDirectory(void)
{TR;
	QStringList recentsessionfiles=m_ctrl->GetRecentSessionsList();
	
	QString sessiondir;
	if(!recentsessionfiles.isEmpty())
	{
		QFileInfo fi(recentsessionfiles.first());
		sessiondir=fi.absolutePath();
	}
	else
	{
		sessiondir=m_ctrl->GetSessionsDirectory();
	}
	return sessiondir;
}

bool LC7Main::SaveSessionOrSaveAsSession()
{TR;
	QString sessionfile;
	if(!m_ctrl->IsSessionOpen(&sessionfile))
	{
		return false;
	}

	if(!m_ctrl->IsSessionModified())
	{
		return true;
	}

	if(sessionfile.isEmpty() || !m_ctrl->SaveSession(sessionfile))
	{
		QString sessiondir = m_ctrl->GetSessionsDirectory();

		sessionfile = QFileDialog::getSaveFileName(this, "Save Session As...", sessiondir, "LC7 Session Files (*.lc7)", 0, CLC7App::getInstance()->GetAttachServer().isListening() ? QFileDialog::DontUseNativeDialog : QFileDialog::Options());
		if(sessionfile.isEmpty())
		{
			return false;
		}
		if(!m_ctrl->SaveSession(sessionfile))
		{
return false;
		}
	}

	GetWorkQueueWidget()->AppendToActivityLog("Session saved.\n");
	return true;
}

bool LC7Main::SaveIfModified(QString message)
{
	TR;
	if (m_ctrl->IsSessionModified())
	{
		ShadeUI(true);
		int res = QMessageBox::question(NULL, "Save this session?", QString("%1\nDo you wish to save it?").arg(message), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		ShadeUI(false);
		if (res == QMessageBox::Yes)
		{
			if (!SaveSessionOrSaveAsSession())
			{
				return false;
			}
		}
		if (res == QMessageBox::Cancel)
		{
			return false;
		}
	}

	return true;
}

void LC7Main::slot_dialogNewSession()
{
	TR;

	DoNewSession();
}

void LC7Main::slot_dialogOpenSession(QString sessionfile)
{
	TR;
	DoOpenSession(sessionfile);
}

void LC7Main::slot_dialogCloseWindow()
{
	TR;
	ShowStartupDialog(false);
	UpdateUI();

	// XXX: ?Start a new session automatically, since users wont know what to do?
}

void LC7Main::ShowStartupDialog(bool show)
{
	TR;
	if (show)
	{
		if (!m_pStartupWidget)
		{
			ShadeUI(true);

			m_pStartupWidget = new CLC7StartupWidget(m_custom_startup_dialog_actions, this, m_ctrl);
			m_pStartupWidget->setObjectName("LC7StartupWidget");

			connect(m_pStartupWidget, &CLC7StartupWidget::sig_newSession, this, &LC7Main::slot_dialogNewSession);
			connect(m_pStartupWidget, &CLC7StartupWidget::sig_openSession, this, &LC7Main::slot_dialogOpenSession);
			connect(m_pStartupWidget, &CLC7StartupWidget::sig_closeWindow, this, &LC7Main::slot_dialogCloseWindow);
			m_pStartupWidget->show();

			m_pStartupWidget->move(0, 0);
			m_pStartupWidget->resize(size());

			m_pStartupWidget->setFocus();
		}
	}
	else
	{
		if (m_pStartupWidget)
		{
			ShadeUI(false);

			m_pStartupWidget->deleteLater();
			m_pStartupWidget = NULL;
		}
	}
}


void LC7Main::slot_asyncProc(QSemaphore *s, QString function, QVariant args)
{
	if (function == "ErrorMessage")
	{
		QString title = args.toList().at(0).toString();
		QString msg = args.toList().at(1).toString();
		bool *ret = (bool *)(args.toList().at(2).toULongLong());

		if (m_trayicon.isVisible())
		{
			m_trayicon.showMessage("LC7", "An error has occurred.", QSystemTrayIcon::Critical);
		}

		if (m_ctrl->GetGUILinkage()->GetWorkQueueWidget() != NULL)
		{
			m_ctrl->GetGUILinkage()->GetWorkQueueWidget()->AppendToActivityLog("Error: " + title + "\n" + msg);
		}

		if (Attach::isInteractiveSession())
		{
			ShadeUI(true);
			QMessageBox::critical(NULL, title, msg, QMessageBox::Ok, QMessageBox::NoButton);
			ShadeUI(false);
		}
		
		*ret=true;
	}
	else if(function=="WarningMessage")
	{
		QString title=args.toList().at(0).toString();
		QString msg=args.toList().at(1).toString();
		bool *ret=(bool *)(args.toList().at(2).toULongLong());

		if (m_trayicon.isVisible())
		{
			m_trayicon.showMessage("LC7", "An warning has occurred.", QSystemTrayIcon::Warning);
		}

		if (m_ctrl->GetGUILinkage()->GetWorkQueueWidget() != NULL)
		{
			m_ctrl->GetGUILinkage()->GetWorkQueueWidget()->AppendToActivityLog("Warning: " + title + "\n" + msg);
		}

		if (Attach::isInteractiveSession())
		{
			ShadeUI(true);
			QMessageBox::warning(NULL, title, msg, QMessageBox::Ok, QMessageBox::NoButton);
			ShadeUI(false);

		}
		*ret=true;
	}
	else if(function=="InfoMessage")
	{
		QString title=args.toList().at(0).toString();
		QString msg=args.toList().at(1).toString();
		bool *ret=(bool *)(args.toList().at(2).toULongLong());

		if (m_trayicon.isVisible())
		{
			m_trayicon.showMessage("LC7", "An informational dialog is available.", QSystemTrayIcon::Information);
		}

		if (m_ctrl->GetGUILinkage()->GetWorkQueueWidget() != NULL)
		{
			m_ctrl->GetGUILinkage()->GetWorkQueueWidget()->AppendToActivityLog("Info: " + title + "\n" + msg);
		}

		if (Attach::isInteractiveSession())
		{
			ShadeUI(true);
			QMessageBox::information(NULL, title, msg, QMessageBox::Ok, QMessageBox::NoButton);
			ShadeUI(false);
		}
		*ret=true;
	}
	else if(function=="YesNoBox")
	{
		QString title=args.toList().at(0).toString();
		QString msg=args.toList().at(1).toString();
		bool *ret=(bool *)(args.toList().at(2).toULongLong());

		if (m_trayicon.isVisible())
		{
			m_trayicon.showMessage("LC7", "A question dialog requires your input.", QSystemTrayIcon::Information);
		}

		if (Attach::isInteractiveSession())
		{
			QMessageBox msgBox;
			msgBox.setWindowTitle(title);
			msgBox.setText(msg);
			msgBox.setIcon(QMessageBox::Question);
			msgBox.addButton(QMessageBox::Yes);
			msgBox.addButton(QMessageBox::No);
			msgBox.setTextFormat(Qt::AutoText);
			msgBox.setDefaultButton(QMessageBox::Yes);
			ShadeUI(true);
			int32_t userReply = msgBox.exec();
			ShadeUI(false);

			*ret = (userReply == QMessageBox::Yes);
		}
		else
		{
			*ret = true;
		}
	}
	else if (function == "YesNoBoxWithNeverAgain")
	{
		ILC7Settings *settings = m_ctrl->GetSettings();

		QString title = args.toList().at(0).toString();
		QString msg = args.toList().at(1).toString();
		QString neveragainkey = args.toList().at(2).toString();
		bool *ret = (bool *)(args.toList().at(3).toULongLong());

		if (!settings->contains(QString("_neveragain_:%1").arg(neveragainkey)))
		{
			if (m_trayicon.isVisible())
			{
				m_trayicon.showMessage("LC7", "A question dialog requires your input.", QSystemTrayIcon::Information);
			}

			bool neveragain = false;

			if (Attach::isInteractiveSession())
			{
				QMessageBox msgBox;
				msgBox.setWindowTitle(title);
				msgBox.setText(msg);
				msgBox.setIcon(QMessageBox::Question);
				msgBox.addButton(QMessageBox::Yes);
				msgBox.addButton(QMessageBox::No);
				msgBox.setTextFormat(Qt::AutoText);
				msgBox.setDefaultButton(QMessageBox::Yes);
				QCheckBox dontShowCheckBox("Don't ask this question again");
				dontShowCheckBox.blockSignals(true);
				msgBox.addButton(&dontShowCheckBox, QMessageBox::ResetRole);
				ShadeUI(true);
				int32_t userReply = msgBox.exec();
				ShadeUI(false);

				*ret = (userReply == QMessageBox::Yes);

				neveragain = dontShowCheckBox.isChecked();
			}
			else
			{
				*ret = true;
			}

			if (neveragain)
			{
				settings->setValue(QString("_neveragain_:%1").arg(neveragainkey), *ret);
			}
		}
		else
		{
			*ret = settings->value(QString("_neveragain_:%1").arg(neveragainkey)).toBool();
		}
	}
	else if (function == "AskForPassword")
	{
		QString title = args.toList().at(0).toString();
		QString msg = args.toList().at(1).toString();
		QString *passwd = (QString *)(args.toList().at(2).toULongLong());
		bool *ret = (bool *)(args.toList().at(3).toULongLong());

		if (m_trayicon.isVisible())
		{
			m_trayicon.showMessage("LC7", "A password dialog requires your input.", QSystemTrayIcon::Information);
		}

		if (!Attach::isInteractiveSession())
		{
			*ret = false;
		}
		else
		{
			QInputDialog passwddlg;
			passwddlg.setTextEchoMode(QLineEdit::Password);
			passwddlg.setWindowTitle(title);
			passwddlg.setLabelText(msg);
			ShadeUI(true);
			int res = passwddlg.exec();
			ShadeUI(false);
			if (!res)
			{
				*ret = false;
			}
			else
			{
				*passwd = passwddlg.textValue();
				*ret = true;
			}
		}
	}
	else if (function == "AskForUsernameAndPassword")
	{
		QString title = args.toList().at(0).toString();
		QString msg = args.toList().at(1).toString();
		QString *username = (QString *)(args.toList().at(2).toULongLong());
		QString *passwd = (QString *)(args.toList().at(3).toULongLong());
		bool *ret = (bool *)(args.toList().at(4).toULongLong());

		if (m_trayicon.isVisible())
		{
			m_trayicon.showMessage("LC7", "A credentials dialog requires your input.", QSystemTrayIcon::Information);
		}

		if (!Attach::isInteractiveSession())
		{
			*ret = false;
		}
		else
		{
			CLC7LoginDlg passwddlg(nullptr, m_ctrl, title, msg, *username, *passwd);
			ShadeUI(true);
			int res = passwddlg.exec();
			ShadeUI(false);
			if (!res)
			{
				*ret = false;
			}
			else
			{
				*username = passwddlg.Username();
				*passwd = passwddlg.Password();
				*ret = true;
			}
		}
	}
	else if (function == "OpenFileDialog")
	{
		QString title = args.toList().at(0).toString();
		QString starting_folder = args.toList().at(1).toString();
		QString filter = args.toList().at(2).toString();
		QString *filepath = (QString *)(args.toList().at(3).toULongLong());
		bool *ret = (bool *)(args.toList().at(4).toULongLong());

		if (m_trayicon.isVisible())
		{
			m_trayicon.showMessage("LC7", "A file-open dialog requires your input.", QSystemTrayIcon::Information);
		}

		if (!Attach::isInteractiveSession())
		{
			*ret = false;
		}
		else
		{
			QFileDialog qfd(NULL, title, starting_folder);
			qfd.setNameFilters(filter.split(";"));
			if (CLC7App::getInstance()->GetAttachServer().isListening())
			{
				qfd.setOption(QFileDialog::DontUseNativeDialog);
			}

			qfd.setAcceptMode(QFileDialog::AcceptOpen);
			qfd.setFileMode(QFileDialog::ExistingFile);
			qfd.setViewMode(QFileDialog::Detail);
			qfd.setModal(true);
			ShadeUI(true);
			int res = qfd.exec();
			ShadeUI(false);
			if (res)
			{
				QStringList files = qfd.selectedFiles();
				if (files.size() != 1)
				{
					*ret = false;
				}
				else
				{
					*filepath = files.first();
					*ret = true;
				}
			}
			else
			{
				*ret = false;
			}
		}
	}
	else if (function == "OpenFileDialogMultiple")
	{
		QString title = args.toList().at(0).toString();
		QString starting_folder = args.toList().at(1).toString();
		QString filter = args.toList().at(2).toString();
		QStringList *paths = (QStringList *)(args.toList().at(3).toULongLong());
		bool *ret = (bool *)(args.toList().at(4).toULongLong());

		if (m_trayicon.isVisible())
		{
			m_trayicon.showMessage("LC7", "A file-open dialog requires your input.", QSystemTrayIcon::Information);
		}

		if (!Attach::isInteractiveSession())
		{
			*ret = false;
		}
		else
		{
			QFileDialog qfd(NULL, title, starting_folder);
			qfd.setNameFilters(filter.split(";"));
			if (CLC7App::getInstance()->GetAttachServer().isListening())
			{
				qfd.setOption(QFileDialog::DontUseNativeDialog);
			}

			qfd.setAcceptMode(QFileDialog::AcceptOpen);
			qfd.setFileMode(QFileDialog::ExistingFiles);
			qfd.setViewMode(QFileDialog::Detail);
			qfd.setModal(true);
			ShadeUI(true);
			int res = qfd.exec();
			ShadeUI(false);
			if (res)
			{
				*paths = qfd.selectedFiles();
				*ret = true;
			}
			else
			{
				*ret = false;
			}
		}
	}
	else if (function == "SaveFileDialog")
	{
		QString title = args.toList().at(0).toString();
		QString starting_folder = args.toList().at(1).toString();
		QString filter = args.toList().at(2).toString();
		QString *filepath = (QString *)(args.toList().at(3).toULongLong());
		bool *ret = (bool *)(args.toList().at(4).toULongLong());

		if (m_trayicon.isVisible())
		{
			m_trayicon.showMessage("LC7", "A file-save dialog requires your input.", QSystemTrayIcon::Information);
		}

		if (!Attach::isInteractiveSession())
		{
			*ret = false;
		}
		else
		{
			QFileDialog qfd(NULL, title, starting_folder);
			qfd.setNameFilters(filter.split(";"));
			if (CLC7App::getInstance()->GetAttachServer().isListening())
			{
				qfd.setOption(QFileDialog::DontUseNativeDialog);
			}

			qfd.setAcceptMode(QFileDialog::AcceptSave);
			qfd.setFileMode(QFileDialog::ExistingFile);
			qfd.setViewMode(QFileDialog::Detail);
			qfd.setModal(true);
			ShadeUI(true);
			int res = qfd.exec();
			ShadeUI(false);
			if (res)
			{
				QStringList files = qfd.selectedFiles();
				if (files.size() != 1)
				{
					*ret = false;
				}
				else
				{
					*filepath = files.first();
					*ret = true;
				}
			}
			else
			{
				*ret = false;
			}
		}
	}
	else if (function == "GetDirectoryDialog")
	{
		QString title = args.toList().at(0).toString();
		QString starting_folder = args.toList().at(1).toString();
		QString *filepath = (QString *)(args.toList().at(2).toULongLong());
		bool *ret = (bool *)(args.toList().at(3).toULongLong());

		if (m_trayicon.isVisible())
		{
			m_trayicon.showMessage("LC7", "A directory dialog requires your input.", QSystemTrayIcon::Information);
		}

		if (!Attach::isInteractiveSession())
		{
			*ret = false;
		}
		else
		{
			ShadeUI(true);
			QString directory = QFileDialog::getExistingDirectory(this,
				title,
				starting_folder,
				QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
			ShadeUI(false);

			if (!directory.isNull() && !directory.isEmpty())
			{
				*filepath = directory;
				*ret = true;
			}
			else
			{
				*ret = false;
			}
		}
	}
	else if (function == "Exit")
	{
		bool warn = args.toList().at(0).toBool();

		if (warn && Attach::isInteractiveSession())
		{
			if (m_trayicon.isVisible())
			{
				m_trayicon.showMessage("LC7", "LC7 task complete. Exiting.");
				QTimer::singleShot(5000, this, &LC7Main::close);
			}
			else
			{
				CLC7ExitWarningDlg m_exitdlg(this, 5);
				if (m_exitdlg.exec())
				{
					close();
				}
			}
		}
		else
		{
			close();
		}
	}
	else if (function == "PauseBackgroundSession")
	{
		QString sessionfile = args.toList().at(0).toString();
		bool *ret = (bool *)(args.toList().at(1).toULongLong());

		*ret = DoPauseBackgroundSession(sessionfile);
	}
	else if (function == "ShowStartupDialog")
	{
		bool show = args.toList().at(0).toBool();		
		ShowStartupDialog(show);
	}
	else if(function=="UpdateUI")
	{
		UpdateUI();
	}
	else if (function == "ShadeUI")
	{
		bool shade = args.toList().at(0).toBool();
		ShadeUI(shade);
	}
	else if (function == "RequestNewSession")
	{
		bool *ret = (bool *)(args.toList().at(0).toULongLong());

		*ret = DoNewSession();
	}
	else if (function == "RequestOpenSession")
	{
		QString requested_session = args.toList().at(0).toString();
		bool *ret = (bool *)(args.toList().at(1).toULongLong());

		*ret = DoOpenSession(requested_session);
	}
	else if (function == "RequestCloseSession")
	{
		bool force = args.toList().at(0).toBool();
		bool allow_startup_dialog = args.toList().at(1).toBool();
		bool *ret = (bool *)(args.toList().at(2).toULongLong());

		*ret = DoCloseSession(force, allow_startup_dialog);
	}
	else if(function=="Callback")
	{
		QList<QVariant> largs = args.toList();
		ILC7GUILinkage::GUITHREAD_CALLBACK_TYPE *target=(ILC7GUILinkage::GUITHREAD_CALLBACK_TYPE *)largs.takeFirst().toULongLong();
		(*target)(largs);
	}

	if(s)
	{
		s->release();
	}
}


ILC7ColorManager *LC7Main::GetColorManager(void)
{
	return m_pColorManager;
}

void LC7Main::slot_singleApplicationMessageReceived(const QString &message)
{TR;
	if (message.startsWith("TASK:"))
	{
		QString taskid = message.mid(5);

		DoExecuteTask(taskid, true);
	}
	if (message.startsWith("RUN:"))
	{
		QString sessionfile = message.mid(4);
		DoExecuteManualTask(sessionfile, true);
	}
	else if (message.startsWith("OPEN:"))
	{
		QString sessionfile = message.mid(5);

		DoOpenSession(sessionfile);
	}
}

void LC7Main::slot_processCommandLine()
{TR;

	QString resumefile = ((CLC7App *)qApp)->GetResumeOverride();
	if (!resumefile.isNull())
	{
		bool didopen = DoOpenSession(resumefile);
		if (!didopen)
		{
			Q_ASSERT(0);
			m_ctrl->GetGUILinkage()->ErrorMessage("Couldn't open resume session",
				QString("Couldn't resume background session. Session file may be lost, or at '%1'. Contact support@l0phtcrack.com").arg(resumefile));
			return;
		}
		if (!m_batch_workqueue->StartRequest())
		{
			m_ctrl->GetGUILinkage()->ErrorMessage("Couldn't start resume session",
				QString("Couldn't start batch queue. Contact support@l0phtcrack.com").arg(resumefile));
			return;
		}
		return;
	}

	QCommandLineParser & parser = ((CLC7App *)qApp)->GetCommandLineParser();

	QString taskid = parser.value("task");
	if (!taskid.isNull())
	{
		ILC7Settings *settings = m_ctrl->GetSettings();
		bool show_startup_dialog = settings->value("_ui_:showstartupdialog", true).toBool();
		bool didexecute = DoExecuteTask(taskid, false);
		if (!didexecute && show_startup_dialog)
		{
			ShowStartupDialog(true);
		}
		return;
	}

	if (parser.isSet("run") && parser.positionalArguments().size() == 1)
	{
		QString runsession = parser.positionalArguments().at(0);
		parser.clearPositionalArguments();
		ILC7Settings *settings = m_ctrl->GetSettings();
		bool show_startup_dialog = settings->value("_ui_:showstartupdialog", true).toBool();
		bool didexecute = DoExecuteManualTask(runsession, false);
		if (!didexecute && show_startup_dialog)
		{
			ShowStartupDialog(true);
		}
		return;
	}

	QString cleanup = parser.value("cleanup");
	if (!cleanup.isNull())
	{
		int attempts = 10;
		while (!QFile::remove(cleanup))
		{
			QThread::sleep(1);
			attempts--;
			if (attempts == 0)
			{
				break;
			}
		}
	}

	// Do automatic updater check
	if (m_ctrl->GetSettings()->value("_update_:check_for_updates",true).toBool())
	{
		DoCheckForUpdates(false);
	}
	
	// Process open, etc
	if (parser.positionalArguments().size()==1)
	{
		ILC7Settings *settings = m_ctrl->GetSettings();
		bool show_startup_dialog = settings->value("_ui_:showstartupdialog", true).toBool();
		bool didopen = DoOpenSession(parser.positionalArguments()[0]);
		if (!didopen && show_startup_dialog)
		{
			ShowStartupDialog(true);
		}
		return;
	}
	
	// See if there's autosaves we need to restore
	if (m_ctrl->GetSettings()->value("_core_:autosave", true).toBool())
	{
		if (m_ctrl->IsAutoSaveAvailable())
		{
			if (DoRestoreAutosaveSession())
			{
				return;
			}
		}
	}

	// If all else fails, do the normal thing
	ILC7Settings *settings = m_ctrl->GetSettings();
	bool show_startup_dialog = settings->value("_ui_:showstartupdialog", true).toBool();
	if (show_startup_dialog)
	{
		ShowStartupDialog(true);
	}
}


bool LC7Main::DoExecuteTask(QString taskid, bool interrupting)
{TR;
	ILC7TaskScheduler *sched = m_ctrl->GetTaskScheduler();
	if (sched->IsTaskRunning())
	{
		return false;
	}

	if (Attach::isInteractiveSession() && interrupting)
	{
		if (m_ctrl->GetGUILinkage()->YesNoBox("Task would like to run", "A scheduled task would like to execute now. Press yes to allow the task to run, or no to continue without running the task."))
		{
			return false;
		}
	}

	if (!DoCloseSession(false, false))
	{
		return false;
	}

	ILC7Task *itask = sched->FindScheduledTaskById(taskid);
	if (!itask)
	{
		if (Attach::isInteractiveSession() && interrupting)
		{
			m_ctrl->GetGUILinkage()->ErrorMessage("Couldn't start task", "Couldn't find task. It may have been removed from the system scheduler.");
		}
		return false;
	}

	// Run the task, only quitting after if the task didn't interrupt a running instance.
	if (!m_ctrl->GetTaskScheduler()->RunTask(itask, !interrupting))
	{
		if (Attach::isInteractiveSession() && interrupting)
		{
			m_ctrl->GetGUILinkage()->ErrorMessage("Couldn't start task", "Couldn't execute task. Contact support@l0phtcrack.com.");
		}
		UpdateUI();
		return false;
	}
	
	UpdateUI();

	return true;
}



bool LC7Main::DoExecuteManualTask(QString sessionfile, bool interrupting)
{
	TR;
	if (Attach::isInteractiveSession() && interrupting)
	{
		if (m_ctrl->GetGUILinkage()->YesNoBox("Task would like to run", "A manually scheduled task would like to execute now. Press yes to allow the task to run, or no to continue without running the task."))
		{
			return false;
		}
	}

	if (!DoCloseSession(false, false))
	{
		return false;
	}

	// Open the session
	if (!m_ctrl->RunManualTask(sessionfile, !interrupting))
	{
		if (Attach::isInteractiveSession() && interrupting)
		{
			m_ctrl->GetGUILinkage()->ErrorMessage("Couldn't start manually scheduled task", "Couldn't execute manually scheduled task. Ensure the session file exists and is readable.");
		}
		UpdateUI();
		return false;
	}

	// Run the queue



	UpdateUI();

	return true;
}

void LC7Main::DoMinimizeToSystemTray()
{TR;
	m_trayicon.show();
	hide();
}

void LC7Main::DoSystemTrayMessage(QString msg)
{TR;
	m_trayicon.showMessage("LC7", msg);
}

void LC7Main::slot_systemTray_activated(QSystemTrayIcon::ActivationReason reason)
{TR;
	m_trayicon.hide();

	show();

//	setWindowState(windowState() & ~Qt::WindowFullScreen);
}

void LC7Main::slot_systemTray_messageClicked()
{TR;
	m_trayicon.hide();

	show();

//	setWindowState(windowState() & ~Qt::WindowFullScreen);
}

QWidget *LC7Main::CreatePresetWidget(QWidget *page, QWidget *configwidget, QString preset_group)
{TR;
	QWidget *widget = new CLC7PresetWidget(m_ctrl->GetPresetManager(), page, configwidget, preset_group);
	return widget;
}

void LC7Main::slot_doUpdate(void)
{TR;
#ifndef DISABLE_UPDATER
	m_updater.doUpdate();
#endif
}

void LC7Main::DoCheckForUpdates(bool manual)
{TR;
#ifndef DISABLE_UPDATER
	if (m_ctrl->IsAnyQueueRunning())
	{
		if (manual)
		{
			m_ctrl->GetGUILinkage()->ErrorMessage("Queue is running", "An operation is in progress. Stop the queue or pause it before performing this action.");
		}
		return;
	}
	
	if (m_updater.isRunning())
	{
		return;
	}
	if (manual)
	{
		m_updater.setManualCheck();
	}
	m_updater.start();
#endif
}

void LC7Main::slot_restoreAutosaveSession(void)
{
	DoRestoreAutosaveSession();
}

bool LC7Main::DoRestoreAutosaveSession(void)
{
	CLC7AutosaveDlg dlg(m_ctrl, this);
	ShadeUI(true);
	int res = dlg.exec();
	ShadeUI(false);
	if (res)
	{
		if (m_ctrl->IsAnyQueueRunning())
		{
			m_ctrl->GetGUILinkage()->ErrorMessage("Queue is running", "An operation is in progress. Stop the queue or pause it before performing this action.");
			return false;
		}

		QString oldsessionfile;
		if (m_ctrl->IsSessionOpen(&oldsessionfile))
		{
			if (!SaveIfModified("In order to open another session, the current one must be closed."))
			{
				UpdateUI();
				return false;
			}
			if (!m_ctrl->CloseSession())
			{
				m_ctrl->GetGUILinkage()->ErrorMessage("Couldn't close session", "Session close failed. Contact support@l0phtcrack.com.");
				UpdateUI();
				return false;
			}
		}

		QString path = dlg.autosaveSessionPath();

		QFileInfo fi(path);
		QString filename = fi.fileName();
		
		int lastunder = filename.lastIndexOf('_');
		if (lastunder != -1)
		{
			filename = filename.left(lastunder);
			lastunder = filename.lastIndexOf('_');
			if (lastunder != -1)
			{
				filename = filename.left(lastunder);
			}
			filename += "_Restored.lc7";
		}

		QString sessiondir = m_ctrl->GetSessionsDirectory();
		QDir sessiondirdir(sessiondir);
		if (!m_ctrl->GetGUILinkage()->SaveFileDialog("Save Restored Autosave Session To...", sessiondirdir.absoluteFilePath(filename), "LC7 Session Files (*.lc7)", filename))
		{
			return false;
		}
		
		QFile::remove(filename);
		if (!QFile::copy(path, filename))
		{
			return false;
		} 
		
		if (!DoOpenSession(filename))
		{
			return false;
		}

		QFile::remove(path);

		return true;
	}
	
	return false;
}

void LC7Main::ShadeUI(bool shade)
{
	if (shade)
	{
		if (m_shadecount == 0)
		{
			m_shade = new QWidget(this);
			m_shade->setAttribute(Qt::WA_StyledBackground);
			m_shade->setAutoFillBackground(true);
			m_shade->setStyleSheet("background: rgba(0,0,0,127);");
			m_shade->show();
			m_shade->resize(size());
		}

		m_shadecount++;
	}
	else if (!shade && m_shadecount != 0)
	{
		m_shadecount--;
		if (m_shadecount == 0)
		{
			delete m_shade;
			m_shade = NULL;
		}
	}
}