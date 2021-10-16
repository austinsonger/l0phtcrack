#include<stdafx.h>

CImportUnixSSHGUI::CImportUnixSSHGUI()
{
	TR;
}

CImportUnixSSHGUI::~CImportUnixSSHGUI()
{
	TR;
}


ILC7Interface *CImportUnixSSHGUI::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CImportUnixSSHGUI::GetID()
{
	TR;
	return UUID_IMPORTUNIXSSHGUI;
}


bool CImportUnixSSHGUI::SaveCreds(QString host, UnixSSHImporter::AUTHTYPE authtype, QString username, LC7SecureString password, QString privkeyfile, LC7SecureString privkeypassword,
	UnixSSHImporter::ELEVTYPE elevtype, LC7SecureString sudopassword, LC7SecureString  supassword, QString &error)
{
	TR;

	if (!g_pLinkage->SecureStore("credentials", QString("importunixssh_username_%1").arg(host), username, error))
		return false;

	if (authtype == UnixSSHImporter::PASSWORD)
	{
		QString authtypestr;
		authtypestr = "password";
		if (!g_pLinkage->SecureStore("credentials", QString("importunixssh_authtype_%1").arg(host), authtypestr, error))
			return false;
		if (!g_pLinkage->SecureStore("credentials", QString("importunixssh_password_%1").arg(host), password, error))
			return false;
	}
	else if (authtype == UnixSSHImporter::PUBLICKEY)
	{
		QString authtypestr;
		authtypestr = "publickey";
		if (!g_pLinkage->SecureStore("credentials", QString("importunixssh_authtype_%1").arg(host), authtypestr, error))
			return false;
		if (!g_pLinkage->SecureStore("credentials", QString("importunixssh_privkeyfile_%1").arg(host), privkeyfile, error))
			return false;
		if (!g_pLinkage->SecureStore("credentials", QString("importunixssh_privkeypassword_%1").arg(host), privkeypassword, error))
			return false;
	}
	else
	{
		return false;
	}


	if (elevtype == UnixSSHImporter::NOELEVATION)
	{
		QString elevtypestr;
		elevtypestr = "noelevation";
		if (!g_pLinkage->SecureStore("credentials", QString("importunixssh_elevtype_%1").arg(host), elevtypestr, error))
			return false;
	}
	else if (elevtype == UnixSSHImporter::SUDO)
	{
		QString elevtypestr;
		elevtypestr = "sudo";
		if (!g_pLinkage->SecureStore("credentials", QString("importunixssh_elevtype_%1").arg(host), elevtypestr, error))
			return false;
		if (!g_pLinkage->SecureStore("credentials", QString("importunixssh_sudopassword_%1").arg(host), sudopassword, error))
			return false;
	}
	else if (elevtype == UnixSSHImporter::SU)
	{
		QString elevtypestr;
		elevtypestr = "su";
		if (!g_pLinkage->SecureStore("credentials", QString("importunixssh_elevtype_%1").arg(host), elevtypestr, error))
			return false;
		if (!g_pLinkage->SecureStore("credentials", QString("importunixssh_supassword_%1").arg(host), supassword, error))
			return false;
	}
	else
	{
		return false;
	}



	return true;
}

bool CImportUnixSSHGUI::LoadCreds(QString host, UnixSSHImporter::AUTHTYPE &authtype, QString &username, LC7SecureString  &password, QString &privkeyfile, LC7SecureString &privkeypassword,
	UnixSSHImporter::ELEVTYPE &elevtype, LC7SecureString  &sudopassword, LC7SecureString  &supassword, QString &error)
{
	TR;
	if (!g_pLinkage->SecureLoad("credentials", QString("importunixssh_username_%1").arg(host), username, error))
		return false;

	QString authtypestr;
	if (!g_pLinkage->SecureLoad("credentials", QString("importunixssh_authtype_%1").arg(host), authtypestr, error))
		return false;

	if (authtypestr == "password")
	{
		authtype = UnixSSHImporter::PASSWORD;
		if (!g_pLinkage->SecureLoad("credentials", QString("importunixssh_password_%1").arg(host), password, error))
		{
			return false;
		}
	}
	else if (authtypestr == "publickey")
	{
		authtype = UnixSSHImporter::PUBLICKEY;
		if (!g_pLinkage->SecureLoad("credentials", QString("importunixssh_privkeyfile_%1").arg(host), privkeyfile, error))
		{
			return false;
		}
		if (!g_pLinkage->SecureLoad("credentials", QString("importunixssh_privkeypassword_%1").arg(host), privkeypassword, error))
		{
			privkeypassword = LC7SecureString("", QString("importunixssh_privkeypassword_%1").arg(host));
		}
	}
	else
	{
		return false;
	}


	QString elevtypestr;
	if (!g_pLinkage->SecureLoad("credentials", QString("importunixssh_elevtype_%1").arg(host), elevtypestr, error))
	{
		return false;
	}

	if (elevtypestr == "noelevation")
	{
		elevtype = UnixSSHImporter::NOELEVATION;
	}
	else if (elevtypestr == "sudo")
	{
		elevtype = UnixSSHImporter::SUDO;
		if (!g_pLinkage->SecureLoad("credentials", QString("importunixssh_sudopassword_%1").arg(host), sudopassword, error))
			return false;
	}
	else if (elevtypestr == "su")
	{
		elevtype = UnixSSHImporter::SU;
		if (!g_pLinkage->SecureLoad("credentials", QString("importunixssh_supassword_%1").arg(host), supassword, error))
			return false;
	}
	else
	{
		return false;
	}


	return true;
}


ILC7Component::RETURNCODE CImportUnixSSHGUI::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{
	TR;
	if (command == "gui" && args[0] == "create")
	{
		//QWidget *parent=(QWidget *)(config["parentwidget"].toULongLong());
		QWidget *page = (QWidget *)(config["pagewidget"].toULongLong());

		ILC7Settings *settings = g_pLinkage->GetSettings();
		QMap<QString, QVariant> def_config = settings->value(UUID_IMPORTUNIXPLUGIN.toString() + ":import_unixssh_defaults", QMap<QString, QVariant>()).toMap();

		ImportUnixSSHConfig *widget = new ImportUnixSSHConfig(page, page, def_config, config.contains("simple") ? config["simple"].toBool() : false);
		config["widget"] = QVariant((qulonglong)widget);

		return SUCCESS;
	}
	else if (command == "gui" && args[0] == "store")
	{
		ImportUnixSSHConfig *widget = (ImportUnixSSHConfig *)(config["widget"].toULongLong());

		// Pull all values out of widget
		bool keep_current_accounts = widget->GetKeepCurrentAccounts();
		bool include_non_login = widget->GetIncludeNonLogin();
		bool use_saved_creds = widget->GetUseSavedCreds();
		bool use_saved_default_creds = widget->GetUseSavedDefaultCreds();
		bool use_specific_creds = widget->GetUseSpecificCreds();
		QString host = widget->GetHost();
		QStringList host_history = widget->GetHostHistory();
		bool use_password_auth = widget->GetUsePasswordAuth();
		bool use_public_key_auth = widget->GetUsePublicKeyAuth();
		QString username = widget->GetUsername();
		LC7SecureString password = widget->GetPassword();
		QString private_key_file = widget->GetPrivateKeyFile();
		LC7SecureString private_key_password = widget->GetPrivateKeyPassword();

		bool no_elevation = widget->GetNoElevation();
		bool su_elevation = widget->GetSUElevation();
		LC7SecureString su_password = widget->GetSUPassword();
		bool sudo_elevation = widget->GetSUDOElevation();
		LC7SecureString sudo_password = widget->GetSUDOPassword();
		bool save_creds = widget->GetSaveCredentials();
		bool save_default_creds = widget->GetSaveDefaultCredentials();
		FOURCC hashtype = widget->GetHashType();
		bool limit_accounts = widget->GetLimitAccounts();
		quint32 account_limit = widget->GetAccountLimit();

		// Save all values as config values
		config["keep_current_accounts"] = keep_current_accounts;
		config["include_non_login"] = include_non_login;
		config["use_saved_creds"] = use_saved_creds;
		config["use_saved_default_creds"] = use_saved_default_creds;
		config["use_specific_creds"] = use_specific_creds;
		config["host"] = host;
		config["host_history"] = host_history;
		if (use_specific_creds)
		{
			config["username"] = username;
			
			config["use_password_auth"] = use_password_auth;
			if (use_password_auth)
			{
				QVariant vpassword;
				vpassword.setValue(password);
				config["password"] = vpassword;
			}
			config["use_public_key_auth"] = use_public_key_auth;
			if (use_public_key_auth)
			{
				config["private_key_file"] = private_key_file;
				QVariant vpassword;
				vpassword.setValue(private_key_password);
				config["private_key_password"] = vpassword;
			}
			
			config["no_elevation"] = no_elevation;
			config["sudo_elevation"] = sudo_elevation;
			if (sudo_elevation)
			{
				QVariant vsudo_password;
				vsudo_password.setValue(sudo_password);
				config["sudo_password"] = vsudo_password;
			}
			config["su_elevation"] = su_elevation;
			if (su_elevation)
			{
				QVariant vsu_password;
				vsu_password.setValue(su_password);
				config["su_password"] = vsu_password;
			}

			config["save_creds"] = save_creds;
			config["save_default_creds"] = save_default_creds;
		}
		config["limit_accounts"] = limit_accounts;
		config["account_limit"] = account_limit;
		config["hashtype"] = (int)hashtype;

		QString disp;
		disp = "Unix SSH Import: " + host + " ";

		if (limit_accounts)
		{
			disp += QString("Limit to %1 accounts,").arg(account_limit);
		}
		if (keep_current_accounts)
		{
			disp += "Keep Current Accounts,";
		}
		else
		{
			disp += "Clear Existing Accounts,";
		}
		if (use_saved_creds)
		{
			disp += "Saved Creds";
		}
		else if (use_saved_default_creds)
		{
			disp += "Saved Default Creds";
		}
		else if (use_specific_creds)
		{
			disp += "Username: " + username +", ";
			if (use_password_auth)
			{
				disp += "Password";
			}
			else if (use_public_key_auth)
			{
				disp += "Public Key";
			}
			if (no_elevation)
			{
				disp += ", No Elevation";
			}
			else if (sudo_elevation)
			{
				disp += ", sudo";
			}
			else if (su_elevation)
			{
				disp += ", su";
			}
		}
		if (include_non_login)
		{
			disp += " (include non-login)";
		}

		config["display_string"] = disp;

		QMap<QString, QVariant> def_config = config;
		def_config.remove("widget");
		def_config.remove("password");
		def_config.remove("sudo_password");
		def_config.remove("su_password");

		ILC7Settings *settings = g_pLinkage->GetSettings();
		settings->setValue(UUID_IMPORTUNIXPLUGIN.toString() + ":import_unixssh_defaults", def_config);

		return SUCCESS;
	}
	else if (command == "gui" && args[0] == "queue")
	{
		// Pull credential and/or save credentials
		if (config.contains("use_saved_creds") && config["use_saved_creds"].toBool())
		{
			QString username;
			UnixSSHImporter::AUTHTYPE authtype;
			LC7SecureString password;
			QString privkeyfile;
			LC7SecureString privkeypassword;
			UnixSSHImporter::ELEVTYPE elevtype;
			LC7SecureString sudopassword;
			LC7SecureString supassword;
			if (!LoadCreds(config["host"].toString(), authtype, username, password, privkeyfile, privkeypassword,
				elevtype, sudopassword, supassword, error))
			{
				return FAIL;
			}
			config["username"] = username;
			if (authtype == UnixSSHImporter::PASSWORD)
			{
				config["use_password_auth"] = true;
				QVariant vpassword;
				vpassword.setValue(password);
				config["password"] = vpassword;
			}
			else if (authtype == UnixSSHImporter::PUBLICKEY)
			{
				config["use_public_key_auth"] = true;
				config["private_key_file"] = privkeyfile;
				QVariant vpassword;
				vpassword.setValue(privkeypassword);
				config["private_key_password"] = vpassword;
			}
			else
			{
				return FAIL;
			}
			if (elevtype == UnixSSHImporter::NOELEVATION)
			{
				config["no_elevation"] = true;
			}
			else if (elevtype == UnixSSHImporter::SUDO)
			{
				config["sudo_elevation"] = true;
				
				QVariant vsudo_password;
				vsudo_password.setValue(sudopassword);
				config["sudo_password"] = vsudo_password;
			}
			else if (elevtype == UnixSSHImporter::SU)
			{
				config["su_elevation"] = true;
				QVariant vsupassword;
				vsupassword.setValue(supassword);
				config["su_password"] = vsupassword;
			}
			else
			{
				return FAIL;
			}
		}
		else if (config.contains("use_saved_default_creds") && config["use_saved_default_creds"].toBool())
		{
			QString username;
			UnixSSHImporter::AUTHTYPE authtype;
			LC7SecureString password;
			QString privkeyfile;
			LC7SecureString privkeypassword;
			UnixSSHImporter::ELEVTYPE elevtype;
			LC7SecureString sudopassword;
			LC7SecureString supassword;
			if (!LoadCreds("%%DEFAULT%%", authtype, username, password, privkeyfile, privkeypassword,
				elevtype, sudopassword, supassword, error))
			{
				return FAIL;
			}
			config["username"] = username;
			if (authtype == UnixSSHImporter::PASSWORD)
			{
				config["use_password_auth"] = true;
				QVariant vpassword;
				vpassword.setValue(password);
				config["password"] = vpassword;
			}
			else if (authtype == UnixSSHImporter::PUBLICKEY)
			{
				config["use_public_key_auth"] = true;
				config["private_key_file"] = privkeyfile;
				QVariant vpassword;
				vpassword.setValue(privkeypassword);
				config["private_key_password"] = vpassword;
			}
			else
			{
				return FAIL;
			}
			if (elevtype == UnixSSHImporter::NOELEVATION)
			{
				config["no_elevation"] = true;
			}
			else if (elevtype == UnixSSHImporter::SUDO)
			{
				config["sudo_elevation"] = true;
				QVariant vsudopassword;
				vsudopassword.setValue(sudopassword);
				config["sudo_password"] = vsudopassword;
			}
			else if (elevtype == UnixSSHImporter::SU)
			{
				config["su_elevation"] = true;

				QVariant vsupassword;
				vsupassword.setValue(supassword);
				config["su_password"] = vsupassword;
			}
			else
			{
				return FAIL;
			}
		}
		else if (config.contains("use_specific_creds") && config["use_specific_creds"].toBool())
		{
			UnixSSHImporter::AUTHTYPE authtype;
			if (config["use_password_auth"].toBool())
			{
				authtype = UnixSSHImporter::PASSWORD;
			}
			else if (config["use_public_key_auth"].toBool())
			{
				authtype = UnixSSHImporter::PUBLICKEY;
			}
			else
			{
				return FAIL;
			}

			UnixSSHImporter::ELEVTYPE elevtype;
			if (config["no_elevation"].toBool())
			{
				elevtype = UnixSSHImporter::NOELEVATION;
			}
			else if (config["sudo_elevation"].toBool())
			{
				elevtype = UnixSSHImporter::SUDO;
			}
			else if (config["su_elevation"].toBool())
			{
				elevtype = UnixSSHImporter::SU;
			}
			else
			{
				return FAIL;
			}

			// Save specific creds if checked
			if (config.contains("save_creds") && config["save_creds"].toBool())
			{
				if (!SaveCreds(config["host"].toString(), authtype, config["username"].toString(), config["password"].value<LC7SecureString>(), config["private_key_file"].toString(), config["private_key_password"].value<LC7SecureString>(),
					elevtype, config["sudo_password"].value<LC7SecureString>(), config["su_password"].value<LC7SecureString>(), error))
				{
					return FAIL;
				}
			}

			// Save default creds if checked
			if (config.contains("save_default_creds") && config["save_default_creds"].toBool())
			{
				if (!SaveCreds("%%DEFAULT%%", authtype, config["username"].toString(), config["password"].value<LC7SecureString>(), config["private_key_file"].toString(), config["private_key_password"].value<LC7SecureString>(),
					elevtype, config["sudo_password"].value<LC7SecureString>(), config["su_password"].value<LC7SecureString>(), error))
				{
					return FAIL;
				}
			}
		}

		// Now's a fine time to save off the host history
		QMap<QString, QVariant> def_config = config;
		def_config.remove("widget");
		def_config.remove("password");
		def_config.remove("private_keypassword");
		def_config.remove("sudo_password");
		def_config.remove("su_password");

		QStringList host_history = def_config["host_history"].toStringList();
		QString host = def_config["host"].toString();
		host_history.removeAll(host);
		host_history.prepend(host);
		def_config["host_history"] = host_history;

		ILC7Settings *settings = g_pLinkage->GetSettings();
		settings->setValue(UUID_IMPORTUNIXPLUGIN.toString() + ":import_unixssh_defaults", def_config);

		// Add the queue item
		ILC7WorkQueue *pwq = (ILC7WorkQueue *)(config["workqueue"].toULongLong());
		LC7WorkQueueItem item(UUID_IMPORTUNIXSSH, "import", QStringList(), config,
			QString("Import hashes from remote Linux/BSD/Solaris/AIX system over SSH (%1)").arg(config["display_string"].toString()), true, false);
		pwq->AppendWorkQueueItem(item);
		return SUCCESS;
	}
	return FAIL;
}


bool CImportUnixSSHGUI::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{
	TR;
	return true;
}
