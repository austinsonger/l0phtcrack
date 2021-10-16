#ifndef __INC_CLC7LINKAGE_H
#define __INC_CLC7LINKAGE_H

class CLC7Controller;

class CLC7Linkage:public QObject, public ILC7Linkage
{
	Q_OBJECT;

private:
	CLC7Controller *m_pController;

public:
	CLC7Linkage(CLC7Controller *pController);
	virtual ~CLC7Linkage();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual QStringList GetActiveClientModes();

	virtual ILC7GUILinkage *GetGUILinkage(void);
	virtual ILC7TaskScheduler *GetTaskScheduler();
	virtual ILC7PresetManager *GetPresetManager();
	virtual ILC7HistoricalData *GetHistoricalData();
	virtual ILC7Settings *GetSettings();
	virtual ILC7CPUInformation *GetCPUInformation();
	virtual ILC7SystemMonitor *GetSystemMonitor();

	virtual bool IsSessionOpen(QString * sessionfile = NULL);
	virtual void ReportSessionModified(void);

	virtual void RegisterSessionHandlerFactory(QUuid handler_id,ILC7SessionHandlerFactory *factory);
	virtual void UnregisterSessionHandlerFactory(QUuid handler_id);
	virtual ILC7SessionHandler *GetSessionHandler(QUuid handler_id);
	virtual void RegisterNotifySessionActivity(QUuid handler_id, QObject *callback_object, void (QObject::*callback_function)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *));
	virtual void UnregisterNotifySessionActivity(QUuid handler_id, QObject *callback_object, void (QObject::*callback_function)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *));

	virtual ILC7PluginRegistry *GetPluginRegistry(void);

	virtual bool AddComponent(ILC7Component *component);
	virtual bool RemoveComponent(ILC7Component *component);
	virtual ILC7Component *FindComponentByID(QUuid id);

	virtual ILC7ActionCategory *CreateActionCategory(QString internal_name, QString name, QString desc);
	virtual void RemoveActionCategory(ILC7ActionCategory *act);
	virtual QList<ILC7ActionCategory *> GetActionCategories();

	virtual bool SecureKeyExists(QString section, QString key);
	virtual bool SecureStore(QString section, QString key, QString value, QString & error);
	virtual bool SecureLoad(QString section, QString key, QString &value, QString & error);
	virtual bool SecureStore(QString section, QString key, LC7SecureString value, QString & error);
	virtual bool SecureLoad(QString section, QString key, LC7SecureString &value, QString & error);
	virtual bool SecureStore(QString section, QString key, QByteArray value, QString & error);
	virtual bool SecureLoad(QString section, QString key, QByteArray &value, QString & error);

	virtual QString NewTemporaryDir();
	virtual QString NewTemporaryFile(bool create=true, QString ext=QString());

	virtual QString GetStartupDirectory();
	virtual QString GetSessionsDirectory();
	virtual QString GetReportsDirectory();
	virtual QString GetPluginsDirectory();
	virtual QString GetAppDataDirectory();
	virtual QString GetCacheDirectory();
	virtual QString GetPublicDataDirectory();

	virtual QStringList GetRecentSessionsList();

};

#endif