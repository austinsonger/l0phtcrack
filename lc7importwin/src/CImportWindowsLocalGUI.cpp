#include<stdafx.h>
#include"importlocalconfig.h"

CImportWindowsLocalGUI::CImportWindowsLocalGUI()
{TR;
}

CImportWindowsLocalGUI::~CImportWindowsLocalGUI()
{TR;
}


ILC7Interface *CImportWindowsLocalGUI::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}


QUuid CImportWindowsLocalGUI::GetID()
{TR;
	return UUID_IMPORTWINDOWSLOCALGUI;
}


bool CImportWindowsLocalGUI::SaveCreds(QString username, LC7SecureString password, QString domain, QString &error)
{TR;
	if(!g_pLinkage->SecureStore("credentials", QString("importwindowslocal_username"), username, error))
		return false;
	
	if(!g_pLinkage->SecureStore("credentials", QString("importwindowslocal_password"), password, error))
		return false;
	
	if(!g_pLinkage->SecureStore("credentials", QString("importwindowslocal_domain"), domain, error))
		return false;

	return true;
}

bool CImportWindowsLocalGUI::LoadCreds(QString &username, LC7SecureString &password, QString &domain, QString &error)
{TR;
	if(!g_pLinkage->SecureLoad("credentials", QString("importwindowslocal_username"), username, error))
		return false;
	
	if(!g_pLinkage->SecureLoad("credentials", QString("importwindowslocal_password"), password, error))
		return false;
	
	if(!g_pLinkage->SecureLoad("credentials", QString("importwindowslocal_domain"), domain, error))
		return false;

	return true;
}


ILC7Component::RETURNCODE CImportWindowsLocalGUI::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	if (command == "gui" && args[0] == "create")
	{
		//QWidget *parent=(QWidget *)(config["parentwidget"].toULongLong());
		QWidget *page = (QWidget *)(config["pagewidget"].toULongLong());

		ILC7Settings *settings = g_pLinkage->GetSettings();
		QMap<QString, QVariant> def_config = settings->value(UUID_IMPORTWINPLUGIN.toString() + ":import_local_defaults", QMap<QString, QVariant>()).toMap();

		ImportLocalConfig *widget = new ImportLocalConfig(page, page, def_config, config.contains("simple")?config["simple"].toBool():false);
		
		config["widget"] = QVariant((qulonglong)widget);

		return SUCCESS;
	}
	else if(command=="gui" && args[0]=="store")
	{
		ImportLocalConfig *widget = (ImportLocalConfig *)(config["widget"].toULongLong());
		
		// Pull all values out of widget
		bool keep_current_accounts=widget->GetKeepCurrentAccounts();
		bool include_machine_accounts=widget->GetIncludeMachineAccounts();
		bool use_current_creds=widget->GetUseCurrentCreds();
		bool use_saved_creds=widget->GetUseSavedCreds();
		bool use_specific_creds=widget->GetUseSpecificCreds();
		QString username=widget->GetUsername();
		LC7SecureString password=widget->GetPassword();
		QString domain=widget->GetDomain();
		bool save_creds=widget->GetSaveCredentials();
		bool limit_accounts = widget->GetLimitAccounts();
		quint32 account_limit = widget->GetAccountLimit();

		// Save all values as config values
		config["keep_current_accounts"]=keep_current_accounts;
		config["include_machine_accounts"]=include_machine_accounts;
		config["use_current_creds"]=use_current_creds;
		config["use_saved_creds"]=use_saved_creds;
		config["use_specific_creds"]=use_specific_creds;
		if (use_specific_creds)
		{
			config["username"] = username;
			QVariant vpassword;
			vpassword.setValue(password);
			config["password"] = vpassword;
			config["domain"] = domain;
			config["save_creds"] = save_creds;
		}
		config["limit_accounts"] = limit_accounts;
		config["account_limit"] = account_limit;
		
		QString disp;
		disp="Local Import: ";

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
			disp+="Current Creds, ";
		}
		else if(use_saved_creds)
		{
			disp+="Saved Creds, ";
		}
		else if(use_specific_creds)
		{
			disp+="Username: "+username;
			disp+=" Domain: "+domain;
		}
		if(include_machine_accounts) 
		{
			disp+="(Including machine accounts)"+domain;
		}
		if (limit_accounts)
		{
			disp += QString("(Limit to %1 accounts)").arg(account_limit);
		}
		config["display_string"]=disp;


		QMap<QString,QVariant> def_config=config;
		def_config.remove("widget");
		def_config.remove("password");

		ILC7Settings *settings=g_pLinkage->GetSettings();
		settings->setValue(UUID_IMPORTWINPLUGIN.toString()+":import_local_defaults",def_config);	

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
			if(!LoadCreds(username,password,domain,error))
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
				if(!SaveCreds(config["username"].toString(),config["password"].value<LC7SecureString>(),config["domain"].toString(),error))
				{
					return FAIL;
				}
			}

		}

		ILC7WorkQueue *pwq=(ILC7WorkQueue *)(config["workqueue"].toULongLong());
		LC7WorkQueueItem item(UUID_IMPORTWINDOWSLOCAL, "import", QStringList(), config,
			QString("Import hashes from local Windows system (%1)").arg(config["display_string"].toString()),true,false);
		pwq->AppendWorkQueueItem(item);
		return SUCCESS;
	}
	return FAIL;
}


bool CImportWindowsLocalGUI::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	return true;
}
