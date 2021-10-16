#ifndef __INC_CLC7CONTROLLER_H
#define __INC_CLC7CONTROLLER_H

#include"ILC7Controller.h"

#include"CLC7PluginRegistry.h"
#include"CLC7TaskScheduler.h"
#include"CLC7WorkQueueFactory.h"
#include"CLC7ActionCategory.h"
#include"CLC7Action.h"
#include"CLC7HistoricalData.h"
#include"CLC7Settings.h"
#include"CLC7CoreSettings.h"
#include"CLC7PresetManager.h"
#include"CLC7CPUInformation.h"
#include"CLC7ThermalWatchdog.h"

class CLC7Linkage;

class CLC7Controller:public QObject, public ILC7Controller
{
	Q_OBJECT;

private:

#ifdef _WIN32
	HANDLE m_compile_mutex;
#endif
	QStringList m_active_client_modes;
	ILC7GUILinkage *m_pGUILinkage;
	CLC7Linkage *m_pLinkage;
	CLC7WorkQueueFactory *m_single_workqueuefactory;
	CLC7WorkQueueFactory *m_batch_workqueuefactory;
	ILC7WorkQueue *m_batch_workqueue;
	ILC7WorkQueue *m_single_workqueue;

	QMap<QUuid,ILC7Component *> m_components_by_uuid;
	
	CLC7ActionCategory m_top_level_action_category;
	
	CLC7PluginRegistry *m_plugin_registry;
	CLC7TaskScheduler *m_task_scheduler;
	CLC7HistoricalData *m_historical_data;
	CLC7PresetManager *m_preset_manager;
	CLC7Settings *m_settings;
	CLC7CPUInformation *m_cpuinformation;
	CLC7CoreSettings *m_pCoreSettings;
	CLC7SystemMonitor *m_system_monitor;
	ILC7ActionCategory *m_pSystemCat;
	ILC7Action *m_pCoreSettingsAct;
	CLC7SecureStringSerializer *m_pSecureStringSerializer;
	
	CLC7ThermalWatchdog *m_thermal_watchdog;

	struct ActivityNotifier
	{
		QObject *callback_object;
		void (QObject::*callback_function)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *);

		bool operator==(const ActivityNotifier &other) {
			return callback_object==other.callback_object && callback_function==other.callback_function;
		}
	};
	QMultiMap<QUuid,ActivityNotifier> m_session_activity_notifiers;
	QMap<QUuid,ILC7SessionHandlerFactory *> m_sessionhandlerfactory_by_id;
	QMap<QUuid,ILC7SessionHandler *> m_sessionhandler_by_id;
	QList<ILC7SessionHandler *> m_sessionhandlers;
	QString m_session_path;
	QMap<QString,QVariant> m_session_data;
	QMap<QString, QVariant> m_session_state;
	bool m_session_opened;
	QMap<QUuid, QByteArray> m_session_partials;
	bool m_session_modified;
	
	QString m_strTempRoot;

	bool m_task_quit_on_finish;
	bool m_task_running;
	CLC7Task *m_current_task;

	QTimer m_autosave_timer;
	int m_autosave_counter;
	QString m_last_autosave_file;

public:

	CLC7Controller();
	virtual ~CLC7Controller();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual void SetGUILinkage(ILC7GUILinkage *guilinkage);
	virtual bool Startup(QStringList active_client_modes);
	virtual bool StartupPlugins(void);
	virtual void ShutdownPlugins(void);
	virtual void Shutdown(void);

	virtual QStringList GetActiveClientModes();

	virtual bool NewSession(void);
	virtual bool NewSessionFromData(QByteArray sessiondata);
	virtual bool CloseSession(void);
	virtual bool IsSessionOpen(QString * sessionfile=NULL);
	virtual bool OpenSession(QString sessionfile);
	virtual bool SaveSession(QString sessionfile, bool as_copy=false, bool allow_insecure=false, bool allow_running=false);
	
	virtual bool RunTask(QString taskid, bool quit_on_finish);
	virtual bool IsTaskRunning(void);
	virtual bool RunManualTask(QString sessionfile, bool quit_on_finish);

	virtual QMap<QUuid, QByteArray> GetSessionPartials();
	virtual bool IsSessionModified(void);
	virtual void ReportSessionModified(void);

	virtual bool IsAnyQueueRunning(void);

	virtual void RegisterSessionHandlerFactory(QUuid handler_id, ILC7SessionHandlerFactory *factory);
	virtual void UnregisterSessionHandlerFactory(QUuid handler_id);
	virtual ILC7SessionHandler *GetSessionHandler(QUuid handler_id);
	virtual void RegisterNotifySessionActivity(QUuid handler_id, QObject *callback_object, void (QObject::*callback_function)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *));
	virtual void UnregisterNotifySessionActivity(QUuid handler_id, QObject *callback_object, void (QObject::*callback_function)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *));

	virtual ILC7Linkage *GetLinkage();
	virtual ILC7GUILinkage *GetGUILinkage();
	virtual ILC7PluginRegistry *GetPluginRegistry();
	virtual ILC7TaskScheduler *GetTaskScheduler();
	virtual ILC7PresetManager *GetPresetManager();
	virtual ILC7HistoricalData *GetHistoricalData();
	virtual ILC7Settings *GetSettings();
	virtual ILC7CPUInformation *GetCPUInformation();
	virtual ILC7SystemMonitor *GetSystemMonitor();
	virtual ILC7ThermalWatchdog *GetThermalWatchdog();

	virtual QMap<QString, QVariant> *GetSessionState();

	virtual bool AddComponent(ILC7Component *component);
	virtual bool RemoveComponent(ILC7Component *component);
	virtual ILC7Component *FindComponentByID(QUuid id);

	virtual ILC7ActionCategory *CreateActionCategory(QString internal_name, QString name, QString desc, QIcon icon=QIcon());
	virtual void RemoveActionCategory(ILC7ActionCategory *cat);
	virtual QList<ILC7ActionCategory *> GetActionCategories();

	virtual bool SecureKeyExists(QString section, QString key);
	virtual bool SecureDelete(QString section, QString key, QString & error);
	virtual bool SecureStore(QString section, QString key, QString value, QString & error);
	virtual bool SecureLoad(QString section, QString key, QString &value, QString & error);
	virtual bool SecureStore(QString section, QString key, LC7SecureString value, QString & error);
	virtual bool SecureLoad(QString section, QString key, LC7SecureString &value, QString & error);
	virtual bool SecureStore(QString section, QString key, QByteArray value, QString & error);
	virtual bool SecureLoad(QString section, QString key, QByteArray &value, QString & error);

	virtual QString NewTemporaryDir();
	virtual QString NewTemporaryFile(bool create=true, QString ext = QString());

	virtual QString GetStartupDirectory();
	virtual QString GetSessionsDirectory();
	virtual QString GetReportsDirectory();
	virtual QString GetPluginsDirectory();
	virtual QString GetAppDataDirectory();
	virtual QString GetCacheDirectory();
	virtual QString GetPublicDataDirectory();

	virtual QStringList GetRecentSessionsList();
	virtual bool IsAutoSaveAvailable();
	virtual QFileInfoList GetAutoSavedSessions();


protected:
	virtual bool LoadSessionFromStream(QDataStream & ds);
	virtual void CallSessionActivityNotifiers(ILC7Linkage::SESSION_ACTIVITY activity);
	virtual void UpdateRecentSessionsList(QString sessionfile);

	void StartAutosaveTimer();
	void StopAutosaveTimer();

public slots:
	void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);
	void slot_batchWorkQueueStateChanged(void);
	void slot_doFinishTask(void);
	void slot_autosaveTimer(void);

};




#endif