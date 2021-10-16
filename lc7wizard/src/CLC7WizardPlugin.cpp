#include<stdafx.h>


CLC7WizardPlugin::CLC7WizardPlugin()
{TR;
	m_pWizardCat = NULL;
	m_pWizardAct = NULL;
}

CLC7WizardPlugin::~CLC7WizardPlugin()
{TR;
}


ILC7Interface *CLC7WizardPlugin::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Plugin")
	{
		return this;
	}
	return NULL;
}
	
QUuid CLC7WizardPlugin::GetID()
{TR;
	return UUID_LC7WIZARDPLUGIN;
}

QList<QUuid> CLC7WizardPlugin::GetInternalDependencies()
{TR;
	return QList<QUuid>();
}


bool CLC7WizardPlugin::Activate()
{	
	m_pWizard = new CLC7WizardComponent();
	
	bool bSuccess=true;
	bSuccess &= g_pLinkage->AddComponent(m_pWizard);

	m_pWizardCat=g_pLinkage->CreateActionCategory("wizard","Wizard","LC7 Wizards");

	m_pWizardAct = m_pWizardCat->CreateAction(m_pWizard->GetID(), "wizard", QStringList(),
		"Password Auditing Wizard",
		"Perform guided auditing of Windows and Unix passwords.");

	g_pLinkage->GetGUILinkage()->AddTopMenuItem(m_pWizardAct);
	g_pLinkage->GetGUILinkage()->AddStartupDialogItem(m_pWizardAct);

	if(!bSuccess)
	{
		Deactivate();
		return false;
	}

	return true;
}

bool CLC7WizardPlugin::Deactivate()
{
	TR;
	if (m_pWizardAct)
	{
		g_pLinkage->GetGUILinkage()->RemoveTopMenuItem(m_pWizardAct);
		g_pLinkage->GetGUILinkage()->RemoveStartupDialogItem(m_pWizardAct);
		m_pWizardCat->RemoveAction(m_pWizardAct);
		m_pWizardAct = NULL;
	}
	if (m_pWizardCat)
	{
		g_pLinkage->RemoveActionCategory(m_pWizardCat);
		m_pWizardCat = NULL;
	}

	if (m_pWizard)
	{
		g_pLinkage->RemoveComponent(m_pWizard);
		delete m_pWizard;
		m_pWizard = NULL;
	}

	
	return true;
}


