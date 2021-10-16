#ifndef LC7MAIN_H
#define LC7MAIN_H

#include <QtWidgets/QMainWindow>
#include "ui_LC7Main.h"
#include "CLC7WorkQueueWidget.h"
#include "CLC7StartupWidget.h"
#include "CLC7NavBar.h"
#include "CLC7Updater.h"
#include "Attach.h"
#include "CAttachServer.h"

class ILC7Controller;

class LC7Main : public QMainWindow
{
	friend class CLC7ColorManager;
	friend class CLC7GUILinkage;

	Q_OBJECT

public:
	LC7Main(ILC7Controller *ctrl, QWidget *parent = 0);
	~LC7Main();
	
	void Startup(void); // Call after controller init
	void Shutdown(void); // Call before controller shutdown

	CLC7WorkQueueWidget *GetWorkQueueWidget();

	virtual void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);

	bool AddMainMenuTab(QString strTabTitle, QString strInternalName, QWidget *pMainMenuTab);
	bool RemoveMainMenuTab(QWidget *pMainMenuTab);
	QWidget *GetMainMenuTab(QString strInternalName);
	bool SwitchToMainMenuTab(QString strInternalName);
	void SwitchToFirstTab();

	void AddTopMenuItem(ILC7Action *act);
	void RemoveTopMenuItem(ILC7Action *act);

	void AddStartupDialogItem(ILC7Action *act);
	void RemoveStartupDialogItem(ILC7Action *act);

	void DoMinimizeToSystemTray();
	void DoSystemTrayMessage(QString msg);

	bool PauseOrStopAllQueues(bool allow_stop);

	QAbstractButton *GetHelpButton(void);
	ILC7ColorManager *GetColorManager(void);

	void ShowStartupDialog(bool show);
	QString GetRecentSessionDirectory(void);

	QWidget *CreatePresetWidget(QWidget *page, QWidget *configwidget, QString preset_group);

	void updateRecentSessionsMenu();

public slots:

	void slot_recentSessionsMenu();
	void onMenuPressed();
	void onNewSession();
	void onOpenSession();
	void onSaveSession();
	void onSaveSessionAs();
	void onCloseSession();
	void onFullscreen();
	void onMinimizeToSystemTray();
	void onCheckForUpdates();
	void onCustomAction();

	void onQuit();

	void slot_dialogNewSession();
	void slot_dialogOpenSession(QString sessionfile);
	void slot_dialogCloseWindow();

	void slot_helpButtonClicked();
	void slot_recolorCallback(void);
	void slot_asyncProc(QSemaphore *s, QString function, QVariant args);

	void slot_singleApplicationMessageReceived(const QString &);
	void slot_processCommandLine();

	void slot_doUpdate(void);

	void slot_systemTray_activated(QSystemTrayIcon::ActivationReason reason);
	void slot_systemTray_messageClicked();

	void slot_update_ui_later(void);

	void slot_restoreAutosaveSession();

protected:
	virtual void resizeEvent(QResizeEvent * evt);
	virtual void closeEvent (QCloseEvent * evt);
	virtual void showEvent(QShowEvent *evt);

	bool SaveSessionOrSaveAsSession();
	bool SaveIfModified(QString message);
	bool DoNewSession(void);
	bool DoCloseSession(bool force, bool allow_startup_dialog);
	bool DoOpenSession(QString session);
	bool DoPauseBackgroundSession(QString sessionFile);
	bool DoExecuteTask(QString taskfile, bool interrupting);
	bool DoExecuteManualTask(QString sessionfile, bool interrupting);
	void DoCheckForUpdates(bool manual);
	bool DoRestoreAutosaveSession(void);

	void UpdateUI();
	void UpdateUILater();

	void ShadeUI(bool shade);

private:

	Ui::MainWindow ui;

	QMenu *m_menu;
	QSplitter *m_pSplitter;
	CLC7WorkQueueWidget *m_pWorkQueueWidget;
	CLC7StartupWidget *m_pStartupWidget;
	CLC7ColorManager *m_pColorManager;
	ILC7WorkQueue *m_batch_workqueue;
	ILC7WorkQueue *m_single_workqueue;
	ILC7Controller *m_ctrl;
	QVector<QWidget *> m_main_tab_widgets;
	QMap<QString, QWidget *> m_main_tab_widgets_by_internal_name;
	CLC7NavBar *m_pNavBar;
	QSystemTrayIcon m_trayicon;
	CLC7Updater m_updater;

	QWidget *m_shade;
	int m_shadecount;

	QTimer m_update_ui_later_timer;
	bool m_update_ui_later;

	bool m_in_foreground;
	
	QAction *m_action_new_session;
	QAction *m_action_open_session;
	QMenu *m_menu_recent_sessions;
	QAction *m_action_restore_session;
	QAction *m_action_save_session;
	QAction *m_action_save_session_as;
	QAction *m_action_close_session;
	QAction *m_action_quit;
	QAction *m_action_custom_sep;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	QAction *m_action_fullscreen;
#endif
	QAction *m_action_run_wizard;
	QAction *m_action_check_for_updates;
	QAction *m_action_systray;

	QMap<ILC7Action *, QAction *> m_custom_top_menu_actions;
	QList<ILC7Action *> m_custom_startup_dialog_actions;

};

#endif // LC7_H
