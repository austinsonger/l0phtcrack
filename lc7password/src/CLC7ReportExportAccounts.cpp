#include<stdafx.h>

CLC7ReportExportAccounts::CLC7ReportExportAccounts()
{
	TR;
	g_pLinkage->RegisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7ReportExportAccounts::NotifySessionActivity);
	m_accountlist = NULL;
}

CLC7ReportExportAccounts::~CLC7ReportExportAccounts()
{
	TR;
	g_pLinkage->UnregisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7ReportExportAccounts::NotifySessionActivity);
}

ILC7Interface *CLC7ReportExportAccounts::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CLC7ReportExportAccounts::GetID()
{
	TR;
	return UUID_REPORTEXPORTACCOUNTS;
}

void CLC7ReportExportAccounts::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
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

ILC7Component::RETURNCODE CLC7ReportExportAccounts::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{
	TR;
	if (command == "export")
	{
		CAccountsExport exp(m_accountlist, ctrl);

		exp.setFilename(config["filename"].toString());
		exp.setFormat(config["format"].toString());
		exp.setIncludeStyle(config["include_style"].toBool());

		foreach(QString key, config.keys())
		{
			if (key.startsWith("column_"))
			{
				exp.enableColumn(key.mid(7), config[key].toBool());
			}
		}

		bool cancelled = false;
		if (!exp.DoExport(error, cancelled))
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

bool CLC7ReportExportAccounts::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{
	TR;
	return true;
}
