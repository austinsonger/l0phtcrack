#ifndef __INC_ILC7LINKAGE_H
#define __INC_ILC7LINKAGE_H

#include"core/ILC7Interface.h"

#include<qstring.h>
#include<quuid.h>

class ILC7GUILinkage;
class ILC7Plugin;
class ILC7Component;
class ILC7ActionCategory;
class ILC7TaskScheduler;
class ILC7HistoricalData;
class ILC7Settings;
class ILC7SessionHandlerFactory;
class ILC7SessionHandler;
class ILC7PluginRegistry;
class ILC7PresetManager;
class ILC7SystemMonitor;

class ILC7Linkage:public ILC7Interface
{
public:
	enum SESSION_ACTIVITY
	{
		SESSION_NEW_POST = 0,
		SESSION_OPEN_POST,
		SESSION_SAVE_PRE,
		SESSION_SAVE_POST,
		SESSION_CLOSE_PRE,
	};

protected:
	virtual ~ILC7Linkage() {}

public:


	virtual QStringList GetActiveClientModes() = 0;

	virtual ILC7GUILinkage *GetGUILinkage() = 0;
	virtual ILC7TaskScheduler *GetTaskScheduler() = 0;
	virtual ILC7PresetManager *GetPresetManager() = 0;
	virtual ILC7HistoricalData *GetHistoricalData() = 0;
	virtual ILC7Settings *GetSettings() = 0;
	virtual ILC7CPUInformation *GetCPUInformation() = 0;
	virtual ILC7SystemMonitor *GetSystemMonitor() = 0;

	virtual bool IsSessionOpen(QString * sessionfile = NULL) = 0;
	virtual void ReportSessionModified(void) = 0;

	virtual void RegisterSessionHandlerFactory(QUuid handler_id, ILC7SessionHandlerFactory *factory) = 0;
	virtual void UnregisterSessionHandlerFactory(QUuid handler_id) = 0;
	virtual ILC7SessionHandler *GetSessionHandler(QUuid handler_id) = 0;
	virtual void RegisterNotifySessionActivity(QUuid handler_id, QObject *callback_object, void (QObject::*callback_function)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *)) = 0;
	virtual void UnregisterNotifySessionActivity(QUuid handler_id, QObject *callback_object, void (QObject::*callback_function)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *)) = 0;

	virtual ILC7PluginRegistry *GetPluginRegistry(void) = 0;

	virtual bool AddComponent(ILC7Component *component) = 0;
	virtual bool RemoveComponent(ILC7Component *component) = 0;
	virtual ILC7Component *FindComponentByID(QUuid id) = 0;

	virtual ILC7ActionCategory *CreateActionCategory(QString internal_name, QString name, QString desc) = 0;
	virtual void RemoveActionCategory(ILC7ActionCategory *act) = 0;
	virtual QList<ILC7ActionCategory *> GetActionCategories() = 0;

	virtual bool SecureKeyExists(QString section, QString key) = 0;
	virtual bool SecureStore(QString section, QString key, QString value, QString & error) = 0;
	virtual bool SecureLoad(QString section, QString key, QString &value, QString & error) = 0;
	virtual bool SecureStore(QString section, QString key, LC7SecureString value, QString & error) = 0;
	virtual bool SecureLoad(QString section, QString key, LC7SecureString &value, QString & error) = 0;
	virtual bool SecureStore(QString section, QString key, QByteArray value, QString & error) = 0;
	virtual bool SecureLoad(QString section, QString key, QByteArray &value, QString & error) = 0;

	virtual QString NewTemporaryDir() = 0;
	virtual QString NewTemporaryFile(bool create = true, QString ext = QString()) = 0;

	virtual QString GetStartupDirectory() = 0;
	virtual QString GetSessionsDirectory() = 0;
	virtual QString GetReportsDirectory() = 0;
	virtual QString GetPluginsDirectory() = 0;
	virtual QString GetAppDataDirectory() = 0;
	virtual QString GetCacheDirectory() = 0;
	virtual QString GetPublicDataDirectory() = 0;

	virtual QStringList GetRecentSessionsList() = 0;
};

extern "C"
{
	typedef bool TYPEOF_Register(ILC7Linkage *pLinkage);
	typedef bool TYPEOF_Unregister(void);
}

#endif