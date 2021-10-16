#include<stdafx.h>

CLC7PasswordPlugin::CLC7PasswordPlugin()
{TR;
	m_pAccountListFactory = nullptr;
	m_pPasswordLinkage = nullptr;
}

CLC7PasswordPlugin::~CLC7PasswordPlugin()
{TR;
	if (m_pAccountListFactory || m_pPasswordLinkage )
	{
		TRDBG("Cleanup password plugin didn't happen. Must deactivate");
	}
}
	

ILC7Interface *CLC7PasswordPlugin::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Plugin")
	{
		return this;
	}
	return nullptr;
}

QUuid CLC7PasswordPlugin::GetID()
{TR;
	return UUID_PASSWORDPLUGIN;
}

QList<QUuid> CLC7PasswordPlugin::GetInternalDependencies()
{TR;
	return QList<QUuid>();
}

bool CLC7PasswordPlugin::Activate()
{TR;

	bool bSuccess=true;

	// Register account list session handler factory
	m_pAccountListFactory=new CLC7AccountListFactory();
	g_pLinkage->RegisterSessionHandlerFactory(ACCOUNTLIST_HANDLER_ID, m_pAccountListFactory);
	
	m_pPasswordLinkage = new CLC7PasswordLinkage();
	m_pReportExportAccounts = new CLC7ReportExportAccounts();
	
	bSuccess &= g_pLinkage->AddComponent(m_pPasswordLinkage);
	bSuccess &= g_pLinkage->AddComponent(m_pReportExportAccounts);
	
	if (!bSuccess)
	{
		Deactivate();
		return false;
	}

	return true;
}


bool CLC7PasswordPlugin::Deactivate()
{	TR;

	if (m_pAccountListFactory)
	{
		g_pLinkage->UnregisterSessionHandlerFactory(ACCOUNTLIST_HANDLER_ID);
		delete m_pAccountListFactory;
		m_pAccountListFactory = nullptr;
	}


	if (m_pReportExportAccounts)
	{
		g_pLinkage->RemoveComponent(m_pReportExportAccounts);
		delete m_pReportExportAccounts;
		m_pReportExportAccounts = nullptr;
	}

	if (m_pPasswordLinkage)
	{
		g_pLinkage->RemoveComponent(m_pPasswordLinkage);
		delete m_pPasswordLinkage;
		m_pPasswordLinkage = nullptr;
	}

	return true;
}

