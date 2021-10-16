#include<stdafx.h>

CImportUnixSSH::CImportUnixSSH()
{
	TR;
	g_pLinkage->RegisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CImportUnixSSH::NotifySessionActivity);
	m_accountlist = NULL;
}

CImportUnixSSH::~CImportUnixSSH()
{
	TR;
	g_pLinkage->UnregisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CImportUnixSSH::NotifySessionActivity);
}


ILC7Interface *CImportUnixSSH::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CImportUnixSSH::GetID()
{
	TR;
	return UUID_IMPORTUNIXSSH;
}

void CImportUnixSSH::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
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

ILC7Component::RETURNCODE CImportUnixSSH::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{
	TR;
	if (command == "import")
	{
		if (config.contains("keep_current_accounts") && !config["keep_current_accounts"].toBool())
		{
			m_accountlist->ClearAccounts();
		}

		bool cancelled = false;
		UnixSSHImporter sshimp(m_accountlist, ctrl);

		// Add remediation config
		LC7Remediations valid_remediations;
		valid_remediations.append(LC7Remediation(UUID_IMPORTUNIXSSH, "disable", "Disable Account", config));
		valid_remediations.append(LC7Remediation(UUID_IMPORTUNIXSSH, "force_change", "Force Password Change", config));
		valid_remediations.append(LC7Remediation(UUID_IMPORTUNIXSSH, "lockout", "Lock Out Account", config));
		sshimp.setRemediations(valid_remediations);

		sshimp.setIncludeNonLogin(config["include_non_login"].toBool());
		sshimp.setHost(config["host"].toString());
		sshimp.setUsername(config["username"].toString());
		if (config["use_password_auth"].toBool())
		{
			sshimp.setAuthType(UnixSSHImporter::PASSWORD);
			sshimp.setPassword(config["password"].value<LC7SecureString>().GetString());
		}
		else if (config["use_public_key_auth"].toBool())
		{
			sshimp.setAuthType(UnixSSHImporter::PUBLICKEY);
			sshimp.setPrivateKeyFile(config["private_key_file"].toString());
			sshimp.setPrivateKeyPassword(config["private_key_password"].value<LC7SecureString>().GetString());
		}
		if (config["no_elevation"].toBool())
		{
			sshimp.setElevType(UnixSSHImporter::NOELEVATION);
		}
		else if (config["sudo_elevation"].toBool())
		{
			sshimp.setElevType(UnixSSHImporter::SUDO);
			sshimp.setSUDOPassword(config["use_password_auth"].toBool() ? config["password"].value<LC7SecureString>().GetString() : config["sudo_password"].value<LC7SecureString>().GetString());
		}
		else if (config["su_elevation"].toBool())
		{
			sshimp.setElevType(UnixSSHImporter::SU);
			sshimp.setSUPassword(config["su_password"].value<LC7SecureString>().GetString());
		}
		else
		{
			error = "Unknown elevation type";
			return FAIL;
		}

		if (config.value("limit_accounts", false).toBool())
		{
			quint32 alim = config.value("account_limit", 0).toUInt();
			if (alim)
			{
				sshimp.SetAccountLimit(alim);
			}
		}

 		if (!sshimp.DoImport(config["hashtype"].toInt(), error, cancelled))
		{
			return FAIL;
		}

		if (cancelled)
		{
			return STOPPED;
		}

		return SUCCESS;
	}
	else if (command == "force_change" || command == "disable" || command == "lockout")
	{
		UnixSSHImporter sshimp(m_accountlist, ctrl);
		
		sshimp.setIncludeNonLogin(config["include_non_login"].toBool());
		sshimp.setHost(config["host"].toString());
		sshimp.setUsername(config["username"].toString());
		if (config["use_password_auth"].toBool())
		{
			sshimp.setAuthType(UnixSSHImporter::PASSWORD);
			sshimp.setPassword(config["password"].value<LC7SecureString>().GetString());
		}
		else if (config["use_public_key_auth"].toBool())
		{
			sshimp.setAuthType(UnixSSHImporter::PUBLICKEY);
			sshimp.setPrivateKeyFile(config["private_key_file"].toString());
			sshimp.setPrivateKeyPassword(config["private_key_password"].value<LC7SecureString>().GetString());
		}
		if (config["no_elevation"].toBool())
		{
			sshimp.setElevType(UnixSSHImporter::NOELEVATION);
		}
		else if (config["sudo_elevation"].toBool())
		{
			sshimp.setElevType(UnixSSHImporter::SUDO);
			sshimp.setSUDOPassword(config["use_password_auth"].toBool() ? config["password"].value<LC7SecureString>().GetString() : config["sudo_password"].value<LC7SecureString>().GetString());
		}
		else if (config["su_elevation"].toBool())
		{
			sshimp.setElevType(UnixSSHImporter::SU);
			sshimp.setSUPassword(config["su_password"].value<LC7SecureString>().GetString());
		}
		else
		{
			error = "Unknown elevation type";
			return FAIL;
		}

		if (command == "disable")
		{
			QList<int> accounts_to_disable;
			foreach(QVariant acctvar, config["accounts_to_remediate"].toList())
			{
				accounts_to_disable.append(acctvar.toInt());
			}

			sshimp.setAccountsToRemediate(accounts_to_disable, QList<int>(), QList<int>());
		}
		else if (command == "force_change")
		{
			QList<int> accounts_to_force_change;
			foreach(QVariant acctvar, config["accounts_to_remediate"].toList())
			{
				accounts_to_force_change.append(acctvar.toInt());
			}

			sshimp.setAccountsToRemediate(QList<int>(), accounts_to_force_change, QList<int>());
		}
		else if (command == "lockout")
		{
			QList<int> accounts_to_lockout;
			foreach(QVariant acctvar, config["accounts_to_remediate"].toList())
			{
				accounts_to_lockout.append(acctvar.toInt());
			}

			sshimp.setAccountsToRemediate(QList<int>(), QList<int>(), accounts_to_lockout);
		}
		bool cancelled = false;
		if (!sshimp.DoRemediate(error, cancelled))
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

bool CImportUnixSSH::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{
	TR;
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
