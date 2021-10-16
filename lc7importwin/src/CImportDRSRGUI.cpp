#include<stdafx.h>
#include"importdrsrconfig.h"

CImportDRSRGUI::CImportDRSRGUI()
{
	TR;
}

CImportDRSRGUI::~CImportDRSRGUI()
{
	TR;
}

ILC7Interface *CImportDRSRGUI::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CImportDRSRGUI::GetID()
{
	TR;
	return UUID_IMPORTDRSRGUI;
}


ILC7Component::RETURNCODE CImportDRSRGUI::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{
	TR;
	if (command == "gui" && args[0] == "create")
	{
		//QWidget *parent=(QWidget *)(config["parentwidget"].toULongLong());
		QWidget *page = (QWidget *)(config["pagewidget"].toULongLong());

		ILC7Settings *settings = g_pLinkage->GetSettings();
		QMap<QString, QVariant> def_config = settings->value(UUID_IMPORTWINPLUGIN.toString() + ":import_ntds_defaults", QMap<QString, QVariant>()).toMap();

		ImportNTDSConfig *widget = new ImportNTDSConfig(page, page, def_config);

		config["widget"] = QVariant((qulonglong)widget);

		return SUCCESS;
	}
	else if (command == "gui" && args[0] == "store")
	{
		ImportNTDSConfig *widget = (ImportNTDSConfig *)(config["widget"].toULongLong());

		// Pull all values out of widget
		bool keep_current_accounts = widget->GetKeepCurrentAccounts();
		QString ntds_filename = widget->GetNTDSFilename();
		QString system_filename = widget->GetSYSTEMFilename();
		bool limit_accounts = widget->GetLimitAccounts();
		quint32 account_limit = widget->GetAccountLimit();

		// Save all values as config values
		config["keep_current_accounts"] = keep_current_accounts;
		config["ntds_filename"] = ntds_filename;
		config["system_filename"] = system_filename;
		config["limit_accounts"] = limit_accounts;
		config["account_limit"] = account_limit;

		QString disp;
		disp = "NTDS.DIT Import: ";

		if (keep_current_accounts)
		{
			disp += "Keep Current Accounts,";
		}
		else
		{
			disp += "Clear Existing Accounts,";
		}
		if (limit_accounts)
		{
			disp += QString("Limit to %1 accounts,").arg(account_limit);
		}
		disp += "NTDS.DIT File: " + ntds_filename + ", ";
		disp += "SYSTEM File: " + system_filename;
		config["display_string"] = disp;

		QMap<QString, QVariant> def_config = config;
		def_config.remove("widget");

		ILC7Settings *settings = g_pLinkage->GetSettings();
		settings->setValue(UUID_IMPORTWINPLUGIN.toString() + ":import_ntds_defaults", def_config);

		return SUCCESS;
	}
	else if (command == "gui" && args[0] == "queue")
	{
		ILC7WorkQueue *pwq = (ILC7WorkQueue *)(config["workqueue"].toULongLong());
		LC7WorkQueueItem item(UUID_IMPORTNTDS, "import", QStringList(), config,
			QString("Import domain hashes from NTDS.DIT/SYSTEM file (%1)").arg(config["display_string"].toString()), true, false);
		pwq->AppendWorkQueueItem(item);
		return SUCCESS;
	}
	return FAIL;
}


bool CImportDRSRGUI::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{
	TR;
	return true;
}
