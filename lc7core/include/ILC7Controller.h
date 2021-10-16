#ifndef __INC_ILC7CONTROLLER_H
#define __INC_ILC7CONTROLLER_H

#include<quuid.h>
#include<qstring.h>
#include<qsettings.h>

class ILC7GUILinkage;
class ILC7PluginRegistry;
class ILC7Session;
class ILC7TaskScheduler;
class ILC7HistoricalData;
class ILC7Settings;
class ILC7SessionHandlerFactory;
class ILC7SessionHandler;
class ILC7WorkQueueWidget;
class ILC7Linkage;

class ILC7Controller: public ILC7Interface
{
public:

	virtual ~ILC7Controller() {}
	
	virtual void SetGUILinkage(ILC7GUILinkage *guilinkage)=0;
	virtual bool Startup(QStringList active_client_modes)=0;
	virtual bool StartupPlugins(void) = 0;
	virtual void ShutdownPlugins(void) = 0;
	virtual void Shutdown(void) = 0;

	virtual QStringList GetActiveClientModes()=0;

	virtual bool NewSession(void)=0;
	virtual bool CloseSession(void) = 0;
	virtual bool IsSessionOpen(QString * sessionfile=NULL)=0;
	virtual bool OpenSession(QString sessionfile)=0;
	virtual bool SaveSession(QString sessionfile, bool as_copy = false, bool allow_insecure = false, bool allow_running = false) = 0;
	virtual bool IsSessionModified(void)=0;
	virtual void ReportSessionModified(void)=0;
	virtual bool RunManualTask(QString sessionfile, bool quit_on_finish)=0;

	virtual bool IsAnyQueueRunning(void) = 0;


	virtual void RegisterSessionHandlerFactory(QUuid handler_id, ILC7SessionHandlerFactory *factory)=0;
	virtual void UnregisterSessionHandlerFactory(QUuid handler_id)=0;
	virtual ILC7SessionHandler *GetSessionHandler(QUuid handler_id)=0;
	virtual void RegisterNotifySessionActivity(QUuid handler_id, QObject *callback_object, void (QObject::*callback_function)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))=0;
	virtual void UnregisterNotifySessionActivity(QUuid handler_id, QObject *callback_object, void (QObject::*callback_function)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))=0;

	virtual ILC7Linkage *GetLinkage(void)=0;
	virtual ILC7GUILinkage *GetGUILinkage(void)=0;
	virtual ILC7PluginRegistry *GetPluginRegistry()=0;
	virtual ILC7TaskScheduler *GetTaskScheduler()=0;
	virtual ILC7PresetManager *GetPresetManager()=0;
	virtual ILC7HistoricalData *GetHistoricalData()=0;
	virtual ILC7Settings *GetSettings()=0;
	virtual ILC7CPUInformation *GetCPUInformation()=0;
	virtual ILC7SystemMonitor *GetSystemMonitor()=0;
	virtual ILC7ThermalWatchdog *GetThermalWatchdog()=0;
	
	virtual bool AddComponent(ILC7Component *component)=0;
	virtual bool RemoveComponent(ILC7Component *component)=0;
	virtual ILC7Component *FindComponentByID(QUuid id)=0;

	virtual ILC7ActionCategory *CreateActionCategory(QString internal_name, QString name, QString desc, QIcon icon=QIcon())=0;
	virtual void RemoveActionCategory(ILC7ActionCategory *cat) = 0;
	virtual QList<ILC7ActionCategory *> GetActionCategories()=0;

	virtual bool SecureKeyExists(QString section, QString key)=0;
	virtual bool SecureDelete(QString section, QString key, QString & error) = 0;
	virtual bool SecureStore(QString section, QString key, QString value, QString & error) = 0;
	virtual bool SecureLoad(QString section, QString key, QString &value, QString & error)=0;
	virtual bool SecureStore(QString section, QString key, QByteArray value, QString & error)=0;
	virtual bool SecureLoad(QString section, QString key, QByteArray &value, QString & error)=0;
	
	virtual QString NewTemporaryDir()=0;
	virtual QString NewTemporaryFile(bool create=true, QString ext = QString()) = 0;

	virtual QString GetStartupDirectory() = 0;
	virtual QString GetSessionsDirectory() = 0;
	virtual QString GetReportsDirectory() = 0;
	virtual QString GetPluginsDirectory() = 0;
	virtual QString GetAppDataDirectory() = 0;
	virtual QString GetCacheDirectory() = 0;
	virtual QString GetPublicDataDirectory() = 0;

	virtual QStringList GetRecentSessionsList()=0;
	virtual bool IsAutoSaveAvailable() =0;
	virtual QFileInfoList GetAutoSavedSessions()=0;

};


typedef ILC7Controller *TYPEOF_CreateLC7Controller();


#endif