#include<stdafx.h>
#include"importshadowconfig.h"

CImportShadowGUI::CImportShadowGUI()
{TR;
}

CImportShadowGUI::~CImportShadowGUI()
{TR;
}


ILC7Interface *CImportShadowGUI::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CImportShadowGUI::GetID()
{TR;
	return UUID_IMPORTSHADOWGUI;
}


ILC7Component::RETURNCODE CImportShadowGUI::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	if (command == "gui" && args[0] == "create")
	{
		//QWidget *parent=(QWidget *)(config["parentwidget"].toULongLong());
		QWidget *page = (QWidget *)(config["pagewidget"].toULongLong());

		ILC7Settings *settings = g_pLinkage->GetSettings();
		QMap<QString, QVariant> def_config = settings->value(UUID_IMPORTUNIXPLUGIN.toString() + ":import_shadow_defaults", QMap<QString, QVariant>()).toMap();

		ImportShadowConfig *widget = new ImportShadowConfig(page, page, def_config, config.contains("simple") ? config["simple"].toBool() : false);

		config["widget"] = QVariant((qulonglong)widget);

		return SUCCESS;
	}
	else if (command == "gui" && args[0] == "store")
	{
		ImportShadowConfig *widget = (ImportShadowConfig *)(config["widget"].toULongLong());

		// Pull all values out of widget
		bool keep_current_accounts = widget->GetKeepCurrentAccounts();
		bool include_non_login = widget->GetIncludeNonLogin();
		QString fileformat = widget->GetFileFormat();
		QString file1name = widget->GetFile1Name();
		QString file2name = widget->GetFile2Name();
		QString file3name = widget->GetFile3Name();
		FOURCC hashtype = widget->GetHashType();
		bool limit_accounts = widget->GetLimitAccounts();
		quint32 account_limit = widget->GetAccountLimit();

		// Save all values as config values
		config["keep_current_accounts"] = keep_current_accounts;
		config["include_non_login"] = include_non_login;
		config["fileformat"] = fileformat;
		config["file1name"] = file1name;
		config["file2name"] = file2name;
		config["file3name"] = file3name;
		config["hashtype"] = QVariant((quint32)hashtype);
		config["limit_accounts"] = limit_accounts;
		config["account_limit"] = account_limit;

		QString disp;
		disp = "Unix File Import: ";

		if (keep_current_accounts)
		{
			disp += "Keep Current Accounts,";
		}
		else
		{
			disp += "Clear Existing Accounts,";
		}

		if (include_non_login)
		{
			disp += "Include Non-Login Accounts,";
		}
		if (limit_accounts)
		{
			disp += QString("Limit to %1 accounts,").arg(account_limit);
		}

		
		disp += "File1: " + file1name + ",";
		if (!file2name.isEmpty())
		{
			disp += "File2: " + file2name + ",";
		}
		if (!file3name.isEmpty())
		{
			disp += "File3: " + file3name + ",";
		}
		config["display_string"] = disp;

		QMap<QString, QVariant> def_config = config;
		def_config.remove("widget");

		ILC7Settings *settings = g_pLinkage->GetSettings();
		settings->setValue(UUID_IMPORTUNIXPLUGIN.toString() + ":import_shadow_defaults", def_config);

		return SUCCESS;
	}
	else if (command == "gui" && args[0] == "queue")
	{
		ILC7WorkQueue *pwq = (ILC7WorkQueue *)(config["workqueue"].toULongLong());
		LC7WorkQueueItem item(UUID_IMPORTSHADOW, "import", QStringList(), config,
			QString("Import hashes from passwd/shadow file (%1)").arg(config["display_string"].toString()), true, false);
		pwq->AppendWorkQueueItem(item);
		return SUCCESS;
	}
	return FAIL;
}


bool CImportShadowGUI::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	return true;
}
