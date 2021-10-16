#include<stdafx.h>
#include"importremoteconfig.h"

CImportWindowsRemoteGUI::CImportWindowsRemoteGUI()
{TR;
}

CImportWindowsRemoteGUI::~CImportWindowsRemoteGUI()
{TR;
}


ILC7Interface *CImportWindowsRemoteGUI::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}


QUuid CImportWindowsRemoteGUI::GetID()
{TR;
	return UUID_IMPORTWINDOWSREMOTEGUI;
}


bool CImportWindowsRemoteGUI::SaveCreds(QString host, QString username, LC7SecureString password, QString domain, QString &error)
{TR;
	if(!g_pLinkage->SecureStore("credentials", QString("importwindowsremote_username_%1").arg(host), username, error))
		return false;
	
	if(!g_pLinkage->SecureStore("credentials", QString("importwindowsremote_password_%1").arg(host), password, error))
		return false;
	
	if(!g_pLinkage->SecureStore("credentials", QString("importwindowsremote_domain_%1").arg(host), domain, error))
		return false;

	return true;
}

bool CImportWindowsRemoteGUI::LoadCreds(QString host, QString &username, LC7SecureString &password, QString &domain, QString &error)
{TR;
	if(!g_pLinkage->SecureLoad("credentials", QString("importwindowsremote_username_%1").arg(host), username, error))
		return false;
	
	if(!g_pLinkage->SecureLoad("credentials", QString("importwindowsremote_password_%1").arg(host), password, error))
		return false;
	
	if(!g_pLinkage->SecureLoad("credentials", QString("importwindowsremote_domain_%1").arg(host), domain, error))
		return false;

	return true;
}


ILC7Component::RETURNCODE CImportWindowsRemoteGUI::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{
	TR;
	if (command == "generateremoteagent")
	{
		GenerateRemoteAgent();
		return SUCCESS;
	}
	else if(command=="gui" && args[0]=="create")
	{
		//QWidget *parent=(QWidget *)(config["parentwidget"].toULongLong());
		QWidget *page=(QWidget *)(config["pagewidget"].toULongLong());
		
		ILC7Settings *settings=g_pLinkage->GetSettings();
		QMap<QString,QVariant> def_config = settings->value(UUID_IMPORTWINPLUGIN.toString()+":import_remote_defaults",QMap<QString,QVariant>()).toMap();
	
		ImportRemoteConfig *widget = new ImportRemoteConfig(page, page, def_config, config.contains("simple") ? config["simple"].toBool() : false);
		config["widget"] = QVariant((qulonglong)widget);

		return SUCCESS;
	}
	else if(command=="gui" && args[0]=="store")
	{
		ImportRemoteConfig *widget = (ImportRemoteConfig *)(config["widget"].toULongLong());
		
		// Pull all values out of widget
		bool keep_current_accounts=widget->GetKeepCurrentAccounts();
		bool include_machine_accounts=widget->GetIncludeMachineAccounts();
		bool use_current_creds=widget->GetUseCurrentCreds();
		bool use_saved_creds=widget->GetUseSavedCreds();
		bool use_saved_default_creds=widget->GetUseSavedDefaultCreds();
		bool use_specific_creds=widget->GetUseSpecificCreds();
		QString host=widget->GetHost();
		QStringList host_history=widget->GetHostHistory();
		QString username=widget->GetUsername();
		LC7SecureString password = widget->GetPassword();
		QString domain=widget->GetDomain();
		bool save_creds=widget->GetSaveCredentials();
		bool save_default_creds=widget->GetSaveDefaultCredentials();
		bool limit_accounts = widget->GetLimitAccounts();
		quint32 account_limit = widget->GetAccountLimit();
		quint32 import_mode = widget->GetImportMode();

		// Save all values as config values
		config["keep_current_accounts"]=keep_current_accounts;
		config["include_machine_accounts"]=include_machine_accounts;
		config["use_current_creds"]=use_current_creds;
		config["use_saved_creds"]=use_saved_creds;
		config["use_saved_default_creds"]=use_saved_default_creds;
		config["use_specific_creds"]=use_specific_creds;
		config["host"]=host;
		config["host_history"]=host_history;
		if (use_specific_creds)
		{
			config["username"] = username;
			QVariant vpassword;
			vpassword.setValue(password);
			config["password"] = vpassword;
			config["domain"] = domain;
			config["save_creds"] = save_creds;
			config["save_default_creds"] = save_default_creds;
		}
		config["limit_accounts"] = limit_accounts;
		config["account_limit"] = account_limit;
		config["import_mode"] = import_mode;

		QString disp;
		disp="Remote Import: "+host+" ";

		if(keep_current_accounts)
		{
			disp+="Keep Current Accounts,";
		}
		else
		{
			disp+="Clear Existing Accounts,";
		}
		if(use_current_creds)
		{
			disp+="Current Creds";
		}
		else if(use_saved_creds)
		{
			disp+="Saved Creds";
		}
		else if(use_saved_default_creds)
		{
			disp+="Saved Default Creds";
		}
		else if(use_specific_creds)
		{
			disp+="Username: "+username;
			disp+=" Domain: "+domain;
		}
		if(include_machine_accounts) 
		{
			disp+=" (Including machine accounts)";
		}
		if (limit_accounts)
		{
			disp += QString("(Limit to %1 accounts)").arg(account_limit);
		}
		switch (import_mode)
		{
		case 0:
			disp += " (AD replication first, then SMB agent)";
			break;
		case 1:
			disp += " (SMB agent first, then AD replication)";
			break;
		case 2:
			disp += " (AD replication only)";
			break;
		case 3:
			disp += " (SMB agent only)";
			break;
		}
		config["display_string"] = disp;

		QMap<QString,QVariant> def_config=config;
		def_config.remove("widget");
		def_config.remove("password");
				
		ILC7Settings *settings=g_pLinkage->GetSettings();
		settings->setValue(UUID_IMPORTWINPLUGIN.toString()+":import_remote_defaults",def_config);	

		return SUCCESS;
	}
	else if(command=="gui" && args[0]=="queue")
	{
		// Pull credential and/or save credentials
		if(config.contains("use_saved_creds") && config["use_saved_creds"].toBool())
		{
			QString username;
			LC7SecureString password;
			QString domain;
			if(!LoadCreds(config["host"].toString(),username,password,domain,error))
			{
				return FAIL;
			}
			config["username"]=username;
			QVariant vpassword;
			vpassword.setValue(password);
			config["password"]=vpassword;
			config["domain"]=domain;
		}
		else if(config.contains("use_saved_default_creds") && config["use_saved_default_creds"].toBool())
		{
			QString username;
			LC7SecureString password;
			QString domain;
			if(!LoadCreds("%%DEFAULT%%",username,password,domain,error))
			{
				return FAIL;
			}
			config["username"]=username;
			QVariant vpassword;
			vpassword.setValue(password);
			config["password"]=vpassword;
			config["domain"]=domain;
		}
		else if(config.contains("use_specific_creds") && config["use_specific_creds"].toBool())
		{
			// Save specific creds if checked
			if(config.contains("save_creds") && config["save_creds"].toBool())
			{
				if (!SaveCreds(config["host"].toString(), config["username"].toString(), config["password"].value<LC7SecureString>(), config["domain"].toString(), error))
				{
					return FAIL;
				}
			}

			// Save default creds if checked
			if(config.contains("save_default_creds") && config["save_default_creds"].toBool())
			{
				if (!SaveCreds("%%DEFAULT%%", config["username"].toString(), config["password"].value<LC7SecureString>(), config["domain"].toString(), error))
				{
					return FAIL;
				}
			}
		}

		// Now's a fine time to save off the host history
		QMap<QString,QVariant> def_config=config;
		def_config.remove("widget");
		def_config.remove("password");
		
		QStringList host_history=def_config["host_history"].toStringList();
		QString host=def_config["host"].toString();
		host_history.removeAll(host);
		host_history.prepend(host);
		def_config["host_history"]=host_history;
		
		ILC7Settings *settings=g_pLinkage->GetSettings();
		settings->setValue(UUID_IMPORTWINPLUGIN.toString()+":import_remote_defaults",def_config);	

		// Add the queue item
		ILC7WorkQueue *pwq=(ILC7WorkQueue *)(config["workqueue"].toULongLong());
		LC7WorkQueueItem item(UUID_IMPORTWINDOWSREMOTE, "import", QStringList(), config,
			QString("Import hashes from remote Windows system (%1)").arg(config["display_string"].toString()),true,false);
		pwq->AppendWorkQueueItem(item);
		return SUCCESS;
	}
	return FAIL;
}


bool CImportWindowsRemoteGUI::ValidateCommand(QMap<QString,QVariant> & state, QString command, QStringList args, QMap<QString,QVariant> & config, QString & error)
{TR;
	return true;
}

void CImportWindowsRemoteGUI::GenerateRemoteAgent(void)
{
	GenerateRemoteAgentDlg dlg;
	g_pLinkage->GetGUILinkage()->ShadeUI(true);
	dlg.exec();
	g_pLinkage->GetGUILinkage()->ShadeUI(false);
}