#include<stdafx.h>


CLC7PasswordGUIPlugin::CLC7PasswordGUIPlugin()
{TR;
	m_pAccountsPage= nullptr;
	m_pImportPage= nullptr;
	m_pAuditPage= nullptr;
}

CLC7PasswordGUIPlugin::~CLC7PasswordGUIPlugin()
{TR;

	if(m_pAccountsPage || m_pImportPage || m_pAuditPage)
	{
		TRDBG("Cleanup password gui plugin didn't happen. Must deactivate");
	}
}

ILC7Interface *CLC7PasswordGUIPlugin::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Plugin")
	{
		return this;
	}
	return NULL;
}
	
	
QUuid CLC7PasswordGUIPlugin::GetID()
{TR;
	return UUID_PASSWORDGUIPLUGIN;
}

QList<QUuid> CLC7PasswordGUIPlugin::GetInternalDependencies()
{TR;
	return QList<QUuid>();
}

bool CLC7PasswordGUIPlugin::Activate()
{TR;

	// Create common work queue
	m_pAccountsPage=CreateAccountsPage();
	m_pImportPage=CreateImportPage();
	m_pAuditPage=CreateAuditPage();
	m_pReportExportAccountsGUI = new CLC7ReportExportAccountsGUI();
	m_pCalibrateGUI = new CLC7CalibrateGUI();
	m_pPasswordUIOptions = new CPasswordUIOptions();

	bool bSuccess=true;
	
	bSuccess &= g_pLinkage->AddComponent(m_pReportExportAccountsGUI);
	bSuccess &= g_pLinkage->AddComponent(m_pCalibrateGUI);
	bSuccess &= g_pLinkage->AddComponent(m_pPasswordUIOptions);

	m_pSystemCat = g_pLinkage->CreateActionCategory("system", "System", "Global system options");
	m_pUISettingsAct = m_pSystemCat->CreateAction(m_pPasswordUIOptions->GetID(), "get_options", QStringList(),
		"UI Settings",
		"User interface settings");
	m_pReportsCat = g_pLinkage->CreateActionCategory("reports", "Reports", "Reports for L0phtCrack 7 Audits");
	m_pExportCat = m_pReportsCat->CreateActionCategory("export", "Export", "Export Session Contents");

	m_pCalibrateAct = m_pSystemCat->CreateAction(m_pCalibrateGUI->GetID(), "gui", QStringList(),
		"Perform Calibration",
		"Calibrate Password Cracking Engines");

	m_pExportAccountsAct = m_pExportCat->CreateAction(m_pReportExportAccountsGUI->GetID(), "gui", QStringList(),
		"Export Accounts Table",
		"Export accounts from accounts table to a CSV file, HTML or XML report.");

	g_pLinkage->GetGUILinkage()->AddTopMenuItem(m_pCalibrateAct);

	bSuccess &= g_pLinkage->GetGUILinkage()->AddMainMenuTab("Passwords/Audit", "passwords/audit", m_pAuditPage);
	bSuccess &= g_pLinkage->GetGUILinkage()->AddMainMenuTab("Passwords/Import", "passwords/import", m_pImportPage);
	bSuccess &= g_pLinkage->GetGUILinkage()->AddMainMenuTab("Passwords/Accounts", "passwords/accounts", m_pAccountsPage);

	return bSuccess;
}


bool CLC7PasswordGUIPlugin::Deactivate()
{TR;

	g_pLinkage->GetGUILinkage()->RemoveTopMenuItem(m_pCalibrateAct);

	g_pLinkage->GetGUILinkage()->RemoveMainMenuTab(m_pAccountsPage);
	g_pLinkage->GetGUILinkage()->RemoveMainMenuTab(m_pImportPage);
	g_pLinkage->GetGUILinkage()->RemoveMainMenuTab(m_pAuditPage);

	if (m_pSystemCat)
	{
		if (m_pUISettingsAct)
		{
			m_pSystemCat->RemoveAction(m_pUISettingsAct);
			m_pUISettingsAct = nullptr;
		}
		if (m_pCalibrateAct)
		{
			m_pSystemCat->RemoveAction(m_pCalibrateAct);
			m_pCalibrateAct = nullptr;
		}
		g_pLinkage->RemoveActionCategory(m_pSystemCat);
		m_pSystemCat = nullptr;
	}
	if (m_pReportsCat)
	{
		if (m_pExportCat)
		{
			if (m_pExportAccountsAct)
			{
				m_pExportCat->RemoveAction(m_pExportAccountsAct);
				m_pExportAccountsAct = nullptr;
			}
			m_pReportsCat->RemoveActionCategory(m_pExportCat);
			m_pExportCat = nullptr;
		}
		g_pLinkage->RemoveActionCategory(m_pReportsCat);
		m_pReportsCat = nullptr;
	}

	if (m_pReportExportAccountsGUI)
	{
		g_pLinkage->RemoveComponent(m_pReportExportAccountsGUI);
		delete m_pReportExportAccountsGUI;
		m_pReportExportAccountsGUI = nullptr;
	}

	if (m_pPasswordUIOptions)
	{
		g_pLinkage->RemoveComponent(m_pPasswordUIOptions);
		delete m_pPasswordUIOptions;
		m_pPasswordUIOptions = nullptr;
	}

	delete m_pAccountsPage;
	m_pAccountsPage = nullptr;

	delete m_pImportPage;
	m_pImportPage = nullptr;

	delete m_pAuditPage;
	m_pAuditPage = nullptr;


	return true;
}


