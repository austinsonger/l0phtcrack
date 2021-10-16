#include<stdafx.h>

#undef TR
#define TR

CLC7Linkage::CLC7Linkage(CLC7Controller *pController)
{TR;
	m_pController=pController;
}

CLC7Linkage::~CLC7Linkage()
{TR;
}

ILC7Interface *CLC7Linkage::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Linkage")
	{
		return this;
	}
	return NULL;
}


QStringList CLC7Linkage::GetActiveClientModes()
{TR;
	return m_pController->GetActiveClientModes();
}


ILC7GUILinkage *CLC7Linkage::GetGUILinkage(void)
{TR;
	return m_pController->GetGUILinkage();
}

ILC7TaskScheduler *CLC7Linkage::GetTaskScheduler()
{TR;
	return m_pController->GetTaskScheduler();
}

ILC7PresetManager *CLC7Linkage::GetPresetManager()
{TR;
	return m_pController->GetPresetManager();
}


ILC7HistoricalData *CLC7Linkage::GetHistoricalData()
{TR;
	return m_pController->GetHistoricalData();
}

ILC7Settings *CLC7Linkage::GetSettings()
{TR;
	return m_pController->GetSettings();
}

ILC7CPUInformation *CLC7Linkage::GetCPUInformation()
{
	return m_pController->GetCPUInformation();
}

ILC7SystemMonitor *CLC7Linkage::GetSystemMonitor()
{
	return m_pController->GetSystemMonitor();
}

void CLC7Linkage::RegisterSessionHandlerFactory(QUuid handler_id,ILC7SessionHandlerFactory *factory)
{TR;
	m_pController->RegisterSessionHandlerFactory(handler_id,factory);
}

void CLC7Linkage::UnregisterSessionHandlerFactory(QUuid handler_id)
{TR;
	m_pController->UnregisterSessionHandlerFactory(handler_id);
}

ILC7SessionHandler *CLC7Linkage::GetSessionHandler(QUuid handler_id)
{TR;
	return m_pController->GetSessionHandler(handler_id);
}

void CLC7Linkage::RegisterNotifySessionActivity(QUuid handler_id, QObject *callback_object, void (QObject::*callback_function)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))
{TR;
	m_pController->RegisterNotifySessionActivity(handler_id,callback_object,callback_function);
}

void CLC7Linkage::UnregisterNotifySessionActivity(QUuid handler_id, QObject *callback_object, void (QObject::*callback_function)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))
{TR;
	m_pController->UnregisterNotifySessionActivity(handler_id,callback_object,callback_function);
}

ILC7PluginRegistry *CLC7Linkage::GetPluginRegistry(void)
{TR;
	return m_pController->GetPluginRegistry();
}

bool CLC7Linkage::AddComponent(ILC7Component *component)
{TR;
	return m_pController->AddComponent(component);
}

bool CLC7Linkage::RemoveComponent(ILC7Component *component)
{TR;
	return m_pController->RemoveComponent(component);
}

ILC7Component *CLC7Linkage::FindComponentByID(QUuid id)
{TR;
	return m_pController->FindComponentByID(id);
}



ILC7ActionCategory *CLC7Linkage::CreateActionCategory(QString internal_name, QString name, QString desc)
{TR;
	return m_pController->CreateActionCategory(internal_name,name,desc);
}

void CLC7Linkage::RemoveActionCategory(ILC7ActionCategory *act)
{TR;
	return m_pController->RemoveActionCategory(act);
}

QList<ILC7ActionCategory *> CLC7Linkage::GetActionCategories()
{TR;
	return m_pController->GetActionCategories();
}

bool CLC7Linkage::SecureKeyExists(QString section, QString key)
{TR;
	return m_pController->SecureKeyExists(section, key);
}

bool CLC7Linkage::SecureStore(QString section, QString key, QString value, QString & error)
{TR;
	return m_pController->SecureStore(section, key, value, error);
}

bool CLC7Linkage::SecureLoad(QString section, QString key, QString &value, QString & error)
{TR;
	return m_pController->SecureLoad(section, key, value, error);
}

bool CLC7Linkage::SecureStore(QString section, QString key, LC7SecureString value, QString & error)
{
	TR;
	return m_pController->SecureStore(section, key, value, error);
}

bool CLC7Linkage::SecureLoad(QString section, QString key, LC7SecureString &value, QString & error)
{
	TR;
	return m_pController->SecureLoad(section, key, value, error);
}

bool CLC7Linkage::SecureStore(QString section, QString key, QByteArray value, QString & error)
{TR;
	return m_pController->SecureStore(section, key, value, error);
}

bool CLC7Linkage::SecureLoad(QString section, QString key, QByteArray &value, QString & error)
{TR;
	return m_pController->SecureLoad(section, key, value, error);
}

QString CLC7Linkage::NewTemporaryDir()
{TR;
	return m_pController->NewTemporaryDir();
}

QString CLC7Linkage::NewTemporaryFile(bool create, QString ext)
{TR;
	return m_pController->NewTemporaryFile(create, ext);
}

QString CLC7Linkage::GetStartupDirectory()
{TR;
	return m_pController->GetStartupDirectory();
}

QString CLC7Linkage::GetSessionsDirectory()
{TR;
	return m_pController->GetSessionsDirectory();
}

QString CLC7Linkage::GetReportsDirectory()
{
	TR;
	return m_pController->GetReportsDirectory();
}

QString CLC7Linkage::GetPluginsDirectory()
{TR;
	return m_pController->GetPluginsDirectory();
}

QString CLC7Linkage::GetAppDataDirectory()
{TR;
	return m_pController->GetAppDataDirectory();
}

QString CLC7Linkage::GetCacheDirectory()
{
	TR;
	return m_pController->GetCacheDirectory();
}

QString CLC7Linkage::GetPublicDataDirectory()
{
	TR;
	return m_pController->GetPublicDataDirectory();
}

QStringList CLC7Linkage::GetRecentSessionsList()
{TR;
	return m_pController->GetRecentSessionsList();
}

bool CLC7Linkage::IsSessionOpen(QString * sessionfile)
{
	return m_pController->IsSessionOpen(sessionfile);
}

void CLC7Linkage::ReportSessionModified(void)
{TR;
	m_pController->ReportSessionModified();
}
