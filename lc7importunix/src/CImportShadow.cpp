#include<stdafx.h>

CImportShadow::CImportShadow()
{TR;
	g_pLinkage->RegisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CImportShadow::NotifySessionActivity);
	m_accountlist = NULL;
}

CImportShadow::~CImportShadow()
{TR;
	g_pLinkage->UnregisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CImportShadow::NotifySessionActivity);
}


ILC7Interface *CImportShadow::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CImportShadow::GetID()
{TR;
	return UUID_IMPORTSHADOW;
}

void CImportShadow::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{TR;
	switch (activity)
	{
	case ILC7Linkage::SESSION_OPEN_POST:
	case ILC7Linkage::SESSION_NEW_POST:
		if (handler && handler->GetId() == ACCOUNTLIST_HANDLER_ID)
		{
			m_accountlist = (ILC7AccountList *)handler;
		}
		break;
	case ILC7Linkage::SESSION_CLOSE_PRE:
		if (handler && handler->GetId() == ACCOUNTLIST_HANDLER_ID)
		{
			m_accountlist = NULL;
		}
		break;
	default:
		break;
	}
}

ILC7Component::RETURNCODE CImportShadow::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	if (command == "import")
	{
		if (config.contains("keep_current_accounts") && !config["keep_current_accounts"].toBool())
		{
			m_accountlist->ClearAccounts();
		}

		bool cancelled = false;
		ShadowImporter shadow(m_accountlist, ctrl);

		QStringList filenames;
		if (!config["file1name"].toString().isEmpty())
		{
			filenames.append(config["file1name"].toString());

			if (!config["file2name"].toString().isEmpty())
			{
				filenames.append(config["file2name"].toString());

				if (!config["file3name"].toString().isEmpty())
				{
					filenames.append(config["file3name"].toString());
				}
			}
		}

		if (config.value("limit_accounts", false).toBool())
		{
			quint32 alim = config.value("account_limit", 0).toUInt();
			if (alim)
			{
				shadow.SetAccountLimit(alim);
			}
		}


		if (!shadow.DoImport(config["fileformat"].toString(), filenames, config["hashtype"].toUInt(), 
			config["include_non_login"].toBool() ? 
				(ShadowImporter::IMPORT_FLAGS)0 : 
				(ShadowImporter::IMPORT_FLAGS)(ShadowImporter::exclude_disabled | ShadowImporter::exclude_lockedout), 
			error, cancelled))
		{
			return FAIL;
		}
		
		if (cancelled)
		{
			return STOPPED;
		}

		return SUCCESS;
	}

	return FAIL;
}

bool CImportShadow::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	if (command == "import")
	{
		// Ensure the hash type imported is something we can handle
		FOURCC hashtype = config["hashtype"].toUInt();
		if (hashtype != FOURCC(HASHTYPE_UNIX_BLOWFISH) &&
			hashtype != FOURCC(HASHTYPE_UNIX_DES) &&
			hashtype != FOURCC(HASHTYPE_UNIX_SHA256) &&
			hashtype != FOURCC(HASHTYPE_UNIX_SHA512) &&
			hashtype != FOURCC(HASHTYPE_UNIX_MD5) &&
			hashtype != FOURCC(HASHTYPE_AIX_MD5) &&
			hashtype != FOURCC(HASHTYPE_AIX_SHA1) &&
			hashtype != FOURCC(HASHTYPE_AIX_SHA256) &&
			hashtype != FOURCC(HASHTYPE_AIX_SHA512) &&
			hashtype != FOURCC(HASHTYPE_MAC_SHA1) &&
			hashtype != FOURCC(HASHTYPE_MAC_SHA512) &&
			hashtype != FOURCC(HASHTYPE_MAC_PBKDF2_SHA512)
			)
		{
			error = "Unsupported hash type";
			return false;
		}

		// Verify only matching hashes (or no hashes) have been imported so far
		if (state.contains("hashtypes"))
		{
			if (config.contains("keep_current_accounts") && !config["keep_current_accounts"].toBool())
			{
				//				if(!state.contains("cracked") || !state["cracked"].toBool())
				//				{
				//					error="Import clears hashes without cracking them first. To perform multiple imports, check 'Keep Current Accounts'";
				//					return false;
				//				}
			}
			else
			{
				QList<QVariant> current_hashtypes = state["hashtypes"].toList();
				if (current_hashtypes.size() == 1 && current_hashtypes[0] != config["hashtype"])
				{
					error = "You can't mix these hash types together in the same session.";
					return false;
				}
			}
		}

		// Change the state to say we imported some unix hashes
		QList<QVariant> hashtypes;
		hashtypes << config["hashtype"];
		state["hashtypes"] = hashtypes;

		// Change the state to say we haven't cracked those hashes yet
		state["cracked"] = false;
	}

	return true;
}
