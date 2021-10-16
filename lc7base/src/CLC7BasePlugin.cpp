#include<stdafx.h>


CLC7BasePlugin::CLC7BasePlugin()
{TR;
	m_pQueuePage=NULL;
	m_pReportsPage=NULL;
	m_pSchedulePage=NULL;
	m_pHelpPage=NULL;
}

CLC7BasePlugin::~CLC7BasePlugin()
{TR;

	if(m_pQueuePage || m_pReportsPage || m_pSchedulePage || m_pHelpPage)
	{
		TRDBG("Cleanup base plugin didn't happen. Must deactivate");
	}
}

ILC7Interface *CLC7BasePlugin::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Plugin")
	{
		return this;
	}
	return NULL;
}

	
QUuid CLC7BasePlugin::GetID()
{TR;
	return UUID_BASEPLUGIN;
}

QList<QUuid> CLC7BasePlugin::GetInternalDependencies()
{TR;
	return QList<QUuid>();
}

bool CLC7BasePlugin::Activate()
{TR;

	// Create common work queue
	m_pReportsPage=CreateReportsPage();
	m_pQueuePage=CreateQueuePage();
	m_pSchedulePage=CreateSchedulePage();
	m_pHelpPage=CreateHelpPage();

	bool bSuccess=true;
	
	bSuccess &= g_pLinkage->GetGUILinkage()->AddMainMenuTab("Base/Documentation","base/documentation", m_pHelpPage);
	bSuccess &= g_pLinkage->GetGUILinkage()->AddMainMenuTab("Base/Schedule","base/schedule", m_pSchedulePage);
	bSuccess &= g_pLinkage->GetGUILinkage()->AddMainMenuTab("Base/Queue","base/queue", m_pQueuePage);
	bSuccess &= g_pLinkage->GetGUILinkage()->AddMainMenuTab("Base/Reports","base/reports", m_pReportsPage);

	return bSuccess;
}


bool CLC7BasePlugin::Deactivate()
{	
	g_pLinkage->GetGUILinkage()->RemoveMainMenuTab(m_pReportsPage);
	g_pLinkage->GetGUILinkage()->RemoveMainMenuTab(m_pQueuePage);
	g_pLinkage->GetGUILinkage()->RemoveMainMenuTab(m_pSchedulePage);
	g_pLinkage->GetGUILinkage()->RemoveMainMenuTab(m_pHelpPage);

	delete m_pReportsPage;
	m_pReportsPage=NULL;
	
	delete m_pQueuePage;
	m_pQueuePage=NULL;

	delete m_pSchedulePage;
	m_pSchedulePage=NULL;

	delete m_pHelpPage;
	m_pHelpPage=NULL;
	
	return true;
}


