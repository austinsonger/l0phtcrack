#include<stdafx.h>

CImportSAM::CImportSAM()
{
	TR;
	g_pLinkage->RegisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CImportSAM::NotifySessionActivity);
	m_accountlist = NULL;
}

CImportSAM::~CImportSAM()
{
	TR;
	g_pLinkage->UnregisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CImportSAM::NotifySessionActivity);
}

ILC7Interface *CImportSAM::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CImportSAM::GetID()
{
	TR;
	return UUID_IMPORTSAM;
}

void CImportSAM::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{
	TR;
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

ILC7Component::RETURNCODE CImportSAM::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{
	TR;
	if (command == "import")
	{
		SAMImporter sam(m_accountlist, ctrl);

		if (config.contains("keep_current_accounts") && !config["keep_current_accounts"].toBool())
		{
			m_accountlist->ClearAccounts();
		}

		if (config.value("limit_accounts", false).toBool())
		{
			int alim = config.value("account_limit", 0).toUInt();
			if (alim)
			{
				sam.SetAccountLimit(alim);
			}
		}

		bool cancelled = false;
		if (!sam.DoImport(config["sam_filename"].toString(), config["system_filename"].toString(), error, cancelled))
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

bool CImportSAM::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{
	TR;
	if (command == "import")
	{
		// Verify only windows hashes (or no hashes) have been imported so far
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
				QList<QVariant> hashtypes = state["hashtypes"].toList();
				foreach(QVariant htv, hashtypes)
				{
					fourcc htfcc = htv.toUInt();

					if (htfcc != FOURCC(HASHTYPE_LM) && htfcc != FOURCC(HASHTYPE_NT))
					{
						error = "Can't mix incompatible hash types.";
						return false;
					}
				}

			}
		}

		// Change the state to say we imported some windows hashes
		QList<QVariant> hashtypes;
		hashtypes << FOURCC(HASHTYPE_LM);
		hashtypes << FOURCC(HASHTYPE_NT);

		state["hashtypes"] = hashtypes;

		// Change the state to say we haven't cracked those hashes yet
		state["cracked"] = false;
	}

	return true;
}
