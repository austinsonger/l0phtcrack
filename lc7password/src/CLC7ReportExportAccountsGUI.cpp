#include<stdafx.h>
#include"exportaccountsconfig.h"

CLC7ReportExportAccountsGUI::CLC7ReportExportAccountsGUI()
{
	TR;
}

CLC7ReportExportAccountsGUI::~CLC7ReportExportAccountsGUI()
{
	TR;
}

ILC7Interface *CLC7ReportExportAccountsGUI::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CLC7ReportExportAccountsGUI::GetID()
{
	TR;
	return UUID_REPORTEXPORTACCOUNTSGUI;
}


ILC7Component::RETURNCODE CLC7ReportExportAccountsGUI::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{
	TR;
	if (command == "gui" && args[0] == "create")
	{
		//QWidget *parent=(QWidget *)(config["parentwidget"].toULongLong());
		QWidget *page = (QWidget *)(config["pagewidget"].toULongLong());

		ILC7Settings *settings = g_pLinkage->GetSettings();
		QMap<QString, QVariant> def_config = settings->value(UUID_PASSWORDPLUGIN.toString() + ":export_accounts_default", QMap<QString, QVariant>()).toMap();

		ExportAccountsConfig *widget = new ExportAccountsConfig(page, page, def_config);

		config["widget"] = QVariant((qulonglong)widget);

		return SUCCESS;
	}
	else if (command == "gui" && args[0] == "store")
	{
		ExportAccountsConfig *widget = (ExportAccountsConfig *)(config["widget"].toULongLong());

		config = widget->GetConfig();

		QMap<QString, QVariant> def_config = config;
		def_config.remove("widget");

		ILC7Settings *settings = g_pLinkage->GetSettings();
		settings->setValue(UUID_PASSWORDPLUGIN.toString() + ":export_accounts_default", def_config);

		return SUCCESS;
	}
	else if (command == "gui" && args[0] == "queue")
	{
		ILC7WorkQueue *pwq = (ILC7WorkQueue *)(config["workqueue"].toULongLong());
		LC7WorkQueueItem item(UUID_REPORTEXPORTACCOUNTS, "export", QStringList(), config,
			QString("Export Accounts (%1)").arg(config["display_string"].toString()), true, false);
		pwq->AppendWorkQueueItem(item);
		return SUCCESS;
	}
	return FAIL;
}


bool CLC7ReportExportAccountsGUI::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{
	TR;
	return true;
}
