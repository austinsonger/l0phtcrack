#include<stdafx.h>


CTechniqueJTR::CTechniqueJTR()
{TR;
}

CTechniqueJTR::~CTechniqueJTR ()
{TR;
}

ILC7Interface *CTechniqueJTR::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CTechniqueJTR::GetID()
{TR;
	return UUID_TECHNIQUEJTR;
}


ILC7Component::RETURNCODE CTechniqueJTR::ExecuteCommand(QString command, QStringList args, QMap<QString,QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	if(command=="crack")
	{
		CLC7JTRPlugin *jtrplugin = (CLC7JTRPlugin *)g_pLinkage->GetPluginRegistry()->FindPluginByID(UUID_LC7JTRPLUGIN);
		if (!jtrplugin)
		{
			error = "Plugin failure";
			return FAIL;
		}
			
		CLC7JTRPasswordEngine *engine = jtrplugin->GetPasswordEngine();
		
		return engine->Crack(config, error, ctrl);
	}

	error="Unknown command";
	return FAIL;
}

bool CTechniqueJTR::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	if(command=="crack")
	{
		CLC7JTRPlugin *jtrplugin = (CLC7JTRPlugin *)g_pLinkage->GetPluginRegistry()->FindPluginByID(UUID_LC7JTRPLUGIN);
		if (!jtrplugin)
		{
			error = "Plugin failure";
			return false;
		}

		CLC7JTRPasswordEngine *engine = jtrplugin->GetPasswordEngine();

		return engine->ValidateCrack(state, config, error);
	}
	return true;
}
