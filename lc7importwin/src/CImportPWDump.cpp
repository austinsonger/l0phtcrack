#include<stdafx.h>

CImportPWDump::CImportPWDump()
{TR;
	g_pLinkage->RegisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CImportPWDump::NotifySessionActivity);
	m_accountlist = NULL;
}

CImportPWDump::~CImportPWDump()
{TR;
	g_pLinkage->UnregisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CImportPWDump::NotifySessionActivity);
}


ILC7Interface *CImportPWDump::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CImportPWDump::GetID()
{TR;
	return UUID_IMPORTPWDUMP;
}

void CImportPWDump::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
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


ILC7Component::RETURNCODE CImportPWDump::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	if (command == "import")
	{
		PWDumpImporter pwdump(m_accountlist, ctrl);

		if (config.contains("keep_current_accounts") && !config["keep_current_accounts"].toBool())
		{
			m_accountlist->ClearAccounts();
		}

		if (config.value("limit_accounts",false).toBool())
		{
			int alim = config.value("account_limit", 0).toUInt();
			if (alim)
			{
				pwdump.SetAccountLimit(alim);
			} 
		}

		bool cancelled = false;
		if (!pwdump.DoImport(config["filename"].toString(), error, cancelled))
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

bool CImportPWDump::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	if (command == "import")
	{
		// Verify only windows hashes (or no hashes) have been imported so far
		QSet<fourcc> hashtypes;

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
				QList<QVariant> hashtypeslist = state["hashtypes"].toList();
				foreach(QVariant htv, hashtypeslist)
				{
					fourcc htfcc = htv.toUInt();
					hashtypes.insert(htfcc);
				}
			}
		}

		// Add hash types from this import
		PWDumpImporter pwdump(m_accountlist, nullptr);
		if (!pwdump.GetHashTypes(config["filename"].toString(), hashtypes, error))
		{
			return false;
		}

		// Check hash types
		if (hashtypes.size() == 2)
		{
			bool ok = false;
			if (hashtypes.contains(FOURCC(HASHTYPE_LM)) && hashtypes.contains(FOURCC(HASHTYPE_NT)))
			{
				ok = true;
			}
			else if (hashtypes.contains(FOURCC(HASHTYPE_LM_CHALRESP)) && hashtypes.contains(FOURCC(HASHTYPE_NTLM_CHALRESP)))
			{
				ok = true;
			}
			else if (hashtypes.contains(FOURCC(HASHTYPE_LM_CHALRESP_V2)) && hashtypes.contains(FOURCC(HASHTYPE_NTLM_CHALRESP_V2)))
			{
				ok = true;
			}
			if (!ok)
			{
				error = "Can not mix these types of hashes";
				return false;
			}
		}
		else if (hashtypes.size()==1)
		{
			bool ok = false;
			if (hashtypes.contains(FOURCC(HASHTYPE_LM)) || hashtypes.contains(FOURCC(HASHTYPE_NT)))
			{
				ok = true;
			}
			else if (hashtypes.contains(FOURCC(HASHTYPE_LM_CHALRESP)) || hashtypes.contains(FOURCC(HASHTYPE_NTLM_CHALRESP)))
			{
				ok = true;
			}
			else if (hashtypes.contains(FOURCC(HASHTYPE_LM_CHALRESP_V2)) || hashtypes.contains(FOURCC(HASHTYPE_NTLM_CHALRESP_V2)))
			{
				ok = true;
			}
			if (!ok)
			{
				error = "Can not import this type of hash";
				return false;
			}
		}
		else if (hashtypes.size()>2)
		{
			error = "Can not mix these types of hashes";
			return false;
		}
		
		// Change the state to say we imported some windows hashes
		QList<QVariant> hashtypeslist;
		foreach(fourcc fcc, hashtypes)
		{
			hashtypeslist << fcc;
		}
		
		state["hashtypes"] = hashtypeslist;

		// Change the state to say we haven't cracked those hashes yet
		state["cracked"] = false;
	}

	return true;
}
