#include<stdafx.h>

CImportWindowsLocal::CImportWindowsLocal()
{TR;
	g_pLinkage->RegisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CImportWindowsLocal::NotifySessionActivity);
	m_accountlist=NULL;
}

CImportWindowsLocal::~CImportWindowsLocal()
{TR;
	g_pLinkage->UnregisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CImportWindowsLocal::NotifySessionActivity);
}


ILC7Interface *CImportWindowsLocal::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CImportWindowsLocal::GetID()
{TR;
	return UUID_IMPORTWINDOWSLOCAL;
}

void CImportWindowsLocal::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{TR;
	switch(activity)
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

ILC7Component::RETURNCODE CImportWindowsLocal::ExecuteCommand(QString command, QStringList args, QMap<QString,QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	if(command=="import")
	{
		CImportWindows win(m_accountlist,ctrl);
	
		// Add remediation config
		LC7Remediations valid_remediations;
		valid_remediations.append(LC7Remediation(UUID_IMPORTWINDOWSLOCAL, "force_change", "Force Password Change", config));
		valid_remediations.append(LC7Remediation(UUID_IMPORTWINDOWSLOCAL, "disable", "Disable Account", config));
		win.SetRemediations(valid_remediations);

		if((!config.contains("use_current_creds") || !config["use_current_creds"].toBool()) && config.contains("username") && config.contains("password"))
		{
			QString username=config["username"].toString();

			QString password = config["password"].value<LC7SecureString>().GetString();
			QString domain=config.contains("domain")?config["domain"].toString():"";
			win.SetSpecificUser(username,password,domain);
		}
		bool includemachineaccounts=false;
		if(config.contains("include_machine_accounts"))
		{
			includemachineaccounts=config["include_machine_accounts"].toBool();
		}

		win.SetIncludeMachineAccounts(includemachineaccounts);

		if(config.contains("keep_current_accounts") && !config["keep_current_accounts"].toBool())
		{
			m_accountlist->ClearAccounts();
		}

		if (config.value("limit_accounts", false).toBool())
		{
			quint32 alim = config.value("account_limit", 0).toUInt();
			if (alim)
			{
				win.SetAccountLimit(alim);
			}
		}

		bool cancelled = false;
		if (!win.DoImport(error, cancelled))
		{
			return FAIL;
		}
		if (cancelled)
		{
			return STOPPED;
		}
		
		return SUCCESS;
	}
	else if (command == "force_change" || command == "disable")
	{
		CImportWindows win(m_accountlist, ctrl);
				
		if ((!config.contains("use_current_creds") || !config["use_current_creds"].toBool()) && config.contains("username") && config.contains("password"))
		{
			QString username = config["username"].toString();

			QString password = config["password"].value<LC7SecureString>().GetString();
			QString domain = config.contains("domain") ? config["domain"].toString() : "";
			win.SetSpecificUser(username, password, domain);
		}
		
		if (command == "force_change")
		{
			QList<int> accounts_to_force_change;
			foreach(QVariant acctvar, config["accounts_to_remediate"].toList())
			{
				accounts_to_force_change.append(acctvar.toInt());
			}

			win.SetAccountsToRemediate(accounts_to_force_change, QList<int>());
		}
		else if (command == "disable")
		{
			QList<int> accounts_to_disable;
			foreach(QVariant acctvar, config["accounts_to_remediate"].toList())
			{
				accounts_to_disable.append(acctvar.toInt());
			}

			win.SetAccountsToRemediate(QList<int>(), accounts_to_disable);
		}

		bool cancelled = false;
		if (!win.DoRemediate(error, cancelled))
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

bool CImportWindowsLocal::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	if(command=="import")
	{
		// Verify only windows hashes (or no hashes) have been imported so far
		if(state.contains("hashtypes"))
		{
			if(config.contains("keep_current_accounts") && !config["keep_current_accounts"].toBool())
			{
//				if(!state.contains("cracked") || !state["cracked"].toBool())
//				{
//					error="Import clears hashes without cracking them first. To perform multiple imports, check 'Keep Current Accounts'";
//					return false;
//				}
			}
			else
			{
				QList<QVariant> hashtypes=state["hashtypes"].toList();
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

		state["hashtypes"]=hashtypes;

		// Change the state to say we haven't cracked those hashes yet
		state["cracked"]=false;
	}

	return true;
}
