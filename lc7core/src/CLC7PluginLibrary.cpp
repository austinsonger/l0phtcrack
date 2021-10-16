#include<stdafx.h>


CLC7PluginLibrary::CLC7PluginLibrary()
{TR;
	m_display_name="";
	m_display_version="";
	m_internal_name="";
	m_internal_version=0;
	m_is_system_library=false;
	m_state=ILC7PluginLibrary::STATE::FAILED;
}

CLC7PluginLibrary::~CLC7PluginLibrary()
{TR;
	ReleaseLibraries();
}

ILC7Interface *CLC7PluginLibrary::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7PluginLibrary")
	{
		return this;
	}
	return NULL;
}


bool CLC7PluginLibrary::LoadManifest(QByteArray manifest_json, QString & error)
{TR;
	QJsonParseError err;
	QJsonDocument doc=QJsonDocument::fromJson(manifest_json,&err);
	if(err.error != QJsonParseError::NoError)
	{
		error=err.errorString();
		return false;
	}
	
	try 
	{
		QJsonObject man_obj = doc.object();
		
		if(!man_obj.contains("displayName") || !man_obj.value("displayName").isString())
		{
			error="Missing or unknown value: displayName";
			return false;
		}
		m_display_name = man_obj.value("displayName").toString();

		if(!man_obj.contains("displayVersion") || !man_obj.value("displayVersion").isString())
		{
			error="Missing or unknown value: displayVersion";
			return false;
		}
		m_display_version = man_obj.value("displayVersion").toString();

		if(!man_obj.contains("authorName") || !man_obj.value("authorName").isString())
		{
			error="Missing or unknown value: authorName";
			return false;
		}
		m_author_name = man_obj.value("authorName").toString();

		if(!man_obj.contains("internalName") || !man_obj.value("internalName").isString())
		{
			error="Missing or unknown value: internalName";
			return false;
		}
		m_internal_name = man_obj.value("internalName").toString();

		if(!man_obj.contains("internalVersion") || !man_obj.value("internalVersion").isDouble())
		{
			error="Missing or unknown value: internalVersion";
			return false;
		}
		m_internal_version = man_obj.value("internalVersion").toInt();

		if(!man_obj.contains("operatingSystem") || !man_obj.value("operatingSystem").isString())
		{
			error="Missing or unknown value: operatingSystem";
			return false;
		}
		m_operating_system = man_obj.value("operatingSystem").toString();

		if(!man_obj.contains("updateURL") || !man_obj.value("updateURL").isString())
		{
			error="Missing or unknown value: updateURL";
			return false;
		}
		m_update_url = man_obj.value("updateURL").toString();

		if(!man_obj.contains("isSystemLibrary") || !man_obj.value("isSystemLibrary").isBool())
		{
			error="Missing or unknown value: isSystemLibrary";
			return false;
		}
		m_is_system_library = man_obj.value("isSystemLibrary").toBool(false);

		if(!man_obj.contains("releaseDate") || !man_obj.value("releaseDate").isString())
		{
			error="Missing or unknown value: releaseDate";
			return false;
		}
		m_release_date = QDateTime::fromString(man_obj.value("releaseDate").toString(), Qt::ISODate);
		if(!m_release_date.isValid())
		{
			error="Invalid date format: releaseDate";
			return false;
		}

		QJsonArray dep_arr = man_obj.value("dependencies").toArray();
		foreach(QJsonValue depval, dep_arr )
		{
			QJsonObject dep_obj=depval.toObject();

			Dependency dep;

			if(!dep_obj.contains("internalName") || !dep_obj.value("internalName").isString())
			{
				error="Missing or unknown value in dependency: internalName";
				return false;
			}
			dep.m_internal_name=dep_obj.value("internalName").toString();

			if(!dep_obj.contains("minInternalVersion") || !dep_obj.value("minInternalVersion").isDouble())
			{
				error="Missing or unknown value in dependency: minInternalVersion";
				return false;
			}
			dep.m_min_internal_version=dep_obj.value("minInternalVersion").toInt();

			m_dependencies.append(dep);
		}
	} 
	catch(...)
	{
		error="Malformed JSON in manifest";
		return false;
	}

	m_state	= DEACTIVATED;

	return true;
}

QString CLC7PluginLibrary::GetDisplayName()
{TR;
	return m_display_name;
}

QString CLC7PluginLibrary::GetDisplayVersion()
{TR;
	return m_display_version;
}

QString CLC7PluginLibrary::GetAuthorName()
{TR;
	return m_author_name;
}

QString CLC7PluginLibrary::GetInternalName()
{TR;
	return m_internal_name;
}

quint32 CLC7PluginLibrary::GetInternalVersion()
{TR;
	return m_internal_version;
}

QString CLC7PluginLibrary::GetUpdateURL()
{TR;
	return m_update_url;
}

QString CLC7PluginLibrary::GetOperatingSystem()
{TR;
	return m_operating_system;
}

bool CLC7PluginLibrary::IsSystemLibrary()
{TR;
	return m_is_system_library;
}

QDateTime CLC7PluginLibrary::GetReleaseDate()
{TR;
	return m_release_date;
}

QList<ILC7PluginLibrary::Dependency> CLC7PluginLibrary::GetDependencies()
{TR;
	return m_dependencies;
}

QList<ILC7Plugin *> CLC7PluginLibrary::GetPlugins()
{TR;
	return m_plugins;
}

void CLC7PluginLibrary::AddPlugin(ILC7Plugin *plugin)
{TR;
	if(m_plugins.contains(plugin))
	{
		Q_ASSERT(0);
		return;
	}

	m_plugins.append(plugin);
}

void CLC7PluginLibrary::RemovePlugin(ILC7Plugin *plugin)
{TR;
	m_plugins.removeOne(plugin);
}

ILC7PluginLibrary::STATE CLC7PluginLibrary::GetState()
{TR;
	return m_state;
}

void CLC7PluginLibrary::Fail(QString reason)
{TR;
	m_state=FAILED;
	m_fail_reason=reason;
}

QString CLC7PluginLibrary::GetFailureReason()
{TR;
	return m_fail_reason;
}


void CLC7PluginLibrary::AddLibrary(QLibrary *lib)
{TR;
	m_libraries.append(lib);
}

void CLC7PluginLibrary::ReleaseLibraries(void)
{TR;
	foreach(QLibrary *lib, m_libraries)
	{
		delete lib;
	}
	m_libraries.clear();
}

QList<QLibrary *> CLC7PluginLibrary::GetLibraries()
{TR;
	return m_libraries;
}

bool CLC7PluginLibrary::Activate()
{TR;
	if(m_state==ACTIVATED)
	{
		return true;
	}
	if(m_state==FAILED)
	{
		Q_ASSERT(0);
		return false;
	}

	// Get plugins in internal dependency order
	bool activated;
	do
	{
		activated=false;

		foreach(ILC7Plugin *plug, m_plugins)
		{
			if(m_activated_plugin_ids.contains(plug->GetID()))
			{
				continue;
			}

			bool wait=false;
			foreach(QUuid depid, plug->GetInternalDependencies())
			{
				if(!m_activated_plugin_ids.contains(depid))
				{
					wait=true;
					break;
				}
			}
			if(!wait)
			{
				if(!plug->Activate())
				{
					Deactivate();

					Fail(QString("Plugin failed to activate: %1. Contact plugin developer.").arg(plug->GetID().toString()));
					return false;
				}

				m_activated_plugins.append(plug);
				m_activated_plugin_ids.insert(plug->GetID());

				activated=true;
			}
		}
	} while(activated);

	
	if(m_activated_plugin_ids.size()!=m_plugins.size())
	{
		QStringList sl;
		foreach(ILC7Plugin *plug, m_plugins)
		{
			if(!m_activated_plugin_ids.contains(plug->GetID()))
			{
				sl.append(plug->GetID().toString());
			}
		}

		Deactivate();
		Fail(QString("Plugins failed to activate due to circular dependency: %1. Contact plugin developer.").arg(sl.join(", ")));
		return false;
	}

	m_state=ACTIVATED;
	return true;
}

void CLC7PluginLibrary::Deactivate()
{TR;
	if(m_state==DEACTIVATED)
	{
		return;
	}
	if(m_state==FAILED)
	{
	//	Q_ASSERT(0);
		return;
	}
	
	// Deactivate all activated plugins in reverse order
	QList<ILC7Plugin *> deactivated_plugins;
	foreach(ILC7Plugin *deplug,m_activated_plugins)
	{
		deactivated_plugins.prepend(deplug);
	}
					
	foreach(ILC7Plugin *deplug,deactivated_plugins)
	{
		deplug->Deactivate();
	}

	m_activated_plugins.clear();
	m_activated_plugin_ids.clear();
	
	m_state=DEACTIVATED;
}

void CLC7PluginLibrary::SetFullPath(QString path)
{TR;
	m_full_path = path;
}

QString CLC7PluginLibrary::GetFullPath()
{TR;
	return m_full_path;
}

	