#include<stdafx.h>


CLC7ReportsPlugin::CLC7ReportsPlugin()
{TR;
}

CLC7ReportsPlugin::~CLC7ReportsPlugin()
{TR;
}

ILC7Interface *CLC7ReportsPlugin::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Plugin")
	{
		return this;
	}
	return NULL;
}

	
QUuid CLC7ReportsPlugin::GetID()
{TR;
	return UUID_REPORTSPLUGIN;
}

QList<QUuid> CLC7ReportsPlugin::GetInternalDependencies()
{TR;
	return QList<QUuid>();
}

bool CLC7ReportsPlugin::Activate()
{	
	m_pReports=new CReports();
	m_pReportsGUI=new CReportsGUI();
	
	bool bSuccess=true;
	bSuccess &= g_pLinkage->AddComponent(m_pReports);
	bSuccess &= g_pLinkage->AddComponent(m_pReportsGUI);

	m_pReportsCat=g_pLinkage->CreateActionCategory("reports","Reports","Reports for L0phtCrack 7 Audits");
	
	m_pReportsAct=m_pReportsCat->CreateAction(m_pReportsGUI->GetID(), "gui", QStringList(),
		"LC7 Reports",
		"Generate reports for L0phtCrack 7 audits. Customize, save to PDF, and print reports, including historical trends.");
	
	if(!bSuccess)
	{
		Deactivate();
		return false;
	}

	return true;
}

bool CLC7ReportsPlugin::Deactivate()
{TR;
		
	if (m_pReportsAct)
	{
		m_pReportsCat->RemoveAction(m_pReportsAct);
		m_pReportsAct = NULL;
	}
	if (m_pReportsCat)
	{
		g_pLinkage->RemoveActionCategory(m_pReportsCat);
		m_pReportsCat = NULL;
	}
	if (m_pReports)
	{
		g_pLinkage->RemoveComponent(m_pReports);
		delete m_pReports;
		m_pReports = NULL;
	}
	if (m_pReportsGUI)
	{
		g_pLinkage->RemoveComponent(m_pReportsGUI);
		delete m_pReportsGUI;
		m_pReportsGUI = NULL;
	}
	
	return true;
}


