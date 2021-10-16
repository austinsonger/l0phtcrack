#include<stdafx.h>


CLC7ImportWinPlugin::CLC7ImportWinPlugin()
{TR;
	m_pImportSAM=NULL;
	m_pImportSAMGUI = NULL;
	m_pImportNTDS = NULL;
	m_pImportNTDSGUI = NULL;
	m_pImportPWDump = NULL;
	m_pImportPWDumpGUI = NULL;
	m_pImportWinRemote = NULL;
	m_pImportWinRemoteGUI = NULL;
	m_pImportWinLocal = NULL;
	m_pImportWinLocalGUI = NULL;
	m_pImportCat = NULL;
	m_pLocalCat = NULL;
	m_pRemoteCat = NULL;
	m_pFileCat = NULL;
	m_pLocalImportAct = NULL;
	m_pRemoteImportAct = NULL;
	m_pFileImportAct = NULL;
	m_pSAMImportAct = NULL;
	m_pNTDSImportAct = NULL;

	m_pCommandsCat = NULL;
	m_pGenerateRemoteAgentAct = NULL;

	m_pSystemCat = NULL;
	m_pWindowsImportSettingsAct = NULL;


	m_use_chalresp = false;

}

CLC7ImportWinPlugin::~CLC7ImportWinPlugin()
{TR;
}


ILC7Interface *CLC7ImportWinPlugin::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Plugin")
	{
		return this;
	}
	return NULL;
}


QUuid CLC7ImportWinPlugin::GetID()
{TR;
	return UUID_IMPORTWINPLUGIN;
}

QList<QUuid> CLC7ImportWinPlugin::GetInternalDependencies()
{TR;
	return QList<QUuid>();
}

bool CLC7ImportWinPlugin::UseChallengeResponse()
{
	return m_use_chalresp;
}

bool CLC7ImportWinPlugin::Activate()
{	
	m_use_chalresp = g_pLinkage->GetSettings()->value(UUID_IMPORTWINPLUGIN.toString() + ":enable_chalresp").toBool();

	m_pImportWinRemote=new CImportWindowsRemote();
	m_pImportWinRemoteGUI=new CImportWindowsRemoteGUI();
	m_pImportWinLocal=new CImportWindowsLocal();
	m_pImportWinLocalGUI=new CImportWindowsLocalGUI();
	m_pImportPWDump=new CImportPWDump();
	m_pImportPWDumpGUI = new CImportPWDumpGUI();
	m_pImportSAM = new CImportSAM();
	m_pImportSAMGUI = new CImportSAMGUI();
	m_pImportNTDS = new CImportNTDS();
	m_pImportNTDSGUI = new CImportNTDSGUI();

	bool bSuccess=true;
	bSuccess &= g_pLinkage->AddComponent(m_pImportWinRemote);
	bSuccess &= g_pLinkage->AddComponent(m_pImportWinRemoteGUI);
	bSuccess &= g_pLinkage->AddComponent(m_pImportWinLocal);
	bSuccess &= g_pLinkage->AddComponent(m_pImportWinLocalGUI);
	bSuccess &= g_pLinkage->AddComponent(m_pImportPWDump);
	bSuccess &= g_pLinkage->AddComponent(m_pImportPWDumpGUI);
	bSuccess &= g_pLinkage->AddComponent(m_pImportSAM);
	bSuccess &= g_pLinkage->AddComponent(m_pImportSAMGUI);
	bSuccess &= g_pLinkage->AddComponent(m_pImportNTDS);
	bSuccess &= g_pLinkage->AddComponent(m_pImportNTDSGUI);

	m_pWindowsImportSettings = new CWindowsImportSettings(g_pLinkage);
	if (!g_pLinkage->AddComponent(m_pWindowsImportSettings))
	{
		return false;
	}

	m_pSystemCat = g_pLinkage->CreateActionCategory("system", "System", "Global system options");
	m_pWindowsImportSettingsAct = m_pSystemCat->CreateAction(m_pWindowsImportSettings->GetID(), "get_options", QStringList(),
		"Advanced Windows Import Settings",
		"Advanced settings for the Windows importer.");


	m_pImportCat = g_pLinkage->CreateActionCategory("import","Import","Import password hashes to audit");
	m_pLocalCat = m_pImportCat->CreateActionCategory("local","Local","Import from local machine");
	m_pRemoteCat = m_pImportCat->CreateActionCategory("remote","Remote","Import from remote machine");
	m_pFileCat = m_pImportCat->CreateActionCategory("file", "File", "Import from file");

	m_pLocalImportAct = m_pLocalCat->CreateAction(m_pImportWinLocalGUI->GetID(), "gui", QStringList(),
		"Import from local Windows system",
		"Import password hashes from this Windows system. Imports local accounts only unless this system is a domain controller.");

	m_pRemoteImportAct = m_pRemoteCat->CreateAction(m_pImportWinRemoteGUI->GetID(), "gui", QStringList(),
		"Import from remote Windows system",
		"Import password hashes from another Windows system. Imports local accounts only unless the chosen system is a domain controller.");
	
	m_pFileImportAct = m_pFileCat->CreateAction(m_pImportPWDumpGUI->GetID(), "gui", QStringList(),
		"Import from PWDump file",
		"Import password hashes from a Windows PWDump format file. Imports Windows LM/NTLM hashes from pwdump, fgdump or others. Also supports 'colon format' files for sniffed challenge/reponse output for LM, NTLM, LMv2 and NTLMv2 negotiations. This is compatible with output from Spiderlabs Responder, and other similar sniffers.");

	m_pSAMImportAct = m_pFileCat->CreateAction(m_pImportSAMGUI->GetID(), "gui", QStringList(),
		"Import from SAM/SYSTEM files",
		"Import Windows non-domain user password hashes from a Windows SAM/SYSTEM registry backup.");

	m_pNTDSImportAct = m_pFileCat->CreateAction(m_pImportNTDSGUI->GetID(), "gui", QStringList(),
		"Import from NTDS.DIT/SYSTEM files",
		"Import Windows domain user password hashes from a Windows NTDS.DIT file and SYSTEM registry backup. ");

	m_pCommandsCat = g_pLinkage->CreateActionCategory("commands", "Commands", "Commands And Utilities");
	m_pGenerateRemoteAgentAct = m_pCommandsCat->CreateAction(m_pImportWinRemoteGUI->GetID(), "generateremoteagent", QStringList(),
		"Generate Remote Agent",
		"Create an agent with installation instructions for manual installation on a target system.");

	g_pLinkage->GetGUILinkage()->AddTopMenuItem(m_pGenerateRemoteAgentAct);

	// Register account types we can import
	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);

#define REGISTER_HASH_TYPE(TYPE, NAME, PLATFORM, DESC) \
	passlink->RegisterHashType(FOURCC(TYPE), NAME, DESC, "import", PLATFORM, GetID());

	REGISTER_HASH_TYPE(HASHTYPE_LM, "LM", "Windows", "Windows LANMAN-Only Hash");
	REGISTER_HASH_TYPE(HASHTYPE_NT, "NTLM", "Windows", "Windows NTLM-Only Hash");
	
	if (m_use_chalresp)
	{
		REGISTER_HASH_TYPE(HASHTYPE_LM_CHALRESP, "LM/CRv1", "Windows", "Windows LM Challenge/Response v1");
		REGISTER_HASH_TYPE(HASHTYPE_NTLM_CHALRESP, "NTLM/CRv1", "Windows", "Windows NTLM Challenge/Response v1");
		REGISTER_HASH_TYPE(HASHTYPE_LM_CHALRESP_V2, "LM/CRv2", "Windows", "Windows LM Challenge/Response v2");
		REGISTER_HASH_TYPE(HASHTYPE_NTLM_CHALRESP_V2, "NTLM/CRv2", "Windows", "Windows NTLM Challenge/Response v2");
	}

	if(!bSuccess)
	{
		Deactivate();
		return false;
	}

	return true;
}

bool CLC7ImportWinPlugin::Deactivate()
{TR;
	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	if (passlink)
	{
#define UNREGISTER_HASH_TYPE(TYPE) \
	passlink->UnregisterHashType(FOURCC(TYPE), "import", GetID());

		UNREGISTER_HASH_TYPE(HASHTYPE_LM);
		UNREGISTER_HASH_TYPE(HASHTYPE_NT);
		if (m_use_chalresp)
		{
			UNREGISTER_HASH_TYPE(HASHTYPE_LM_CHALRESP);
			UNREGISTER_HASH_TYPE(HASHTYPE_NTLM_CHALRESP);
			UNREGISTER_HASH_TYPE(HASHTYPE_LM_CHALRESP_V2);
			UNREGISTER_HASH_TYPE(HASHTYPE_NTLM_CHALRESP_V2);
		}
	}

	if (m_pSystemCat)
	{
		if (m_pWindowsImportSettingsAct)
		{
			m_pSystemCat->RemoveAction(m_pWindowsImportSettingsAct);
			m_pWindowsImportSettingsAct = NULL;
		}

		g_pLinkage->RemoveActionCategory(m_pSystemCat);
		m_pSystemCat = NULL;
	}

	if (m_pImportCat)
	{
		if (m_pFileCat)
		{
			if (m_pFileImportAct)
			{
				m_pFileCat->RemoveAction(m_pFileImportAct);
				m_pFileImportAct = NULL;
			}
			if (m_pSAMImportAct)
			{
				m_pFileCat->RemoveAction(m_pSAMImportAct);
				m_pSAMImportAct = NULL;
			}
			if (m_pNTDSImportAct)
			{
				m_pFileCat->RemoveAction(m_pNTDSImportAct);
				m_pNTDSImportAct = NULL;
			}
			m_pImportCat->RemoveActionCategory(m_pFileCat);
			m_pFileCat = NULL;
		}
		if (m_pLocalCat)
		{
			if (m_pLocalImportAct)
			{
				m_pLocalCat->RemoveAction(m_pLocalImportAct);
				m_pLocalImportAct = NULL;
			}
			m_pImportCat->RemoveActionCategory(m_pLocalCat);
			m_pLocalCat = NULL;
		}
		if (m_pRemoteCat)
		{
			if (m_pRemoteImportAct)
			{
				m_pRemoteCat->RemoveAction(m_pRemoteImportAct);
				m_pRemoteImportAct = NULL;
			}
			m_pImportCat->RemoveActionCategory(m_pRemoteCat);
			m_pRemoteCat = NULL;
		}

		g_pLinkage->RemoveActionCategory(m_pImportCat);
		m_pImportCat = NULL;
	}

	if (m_pCommandsCat)
	{
		if (m_pGenerateRemoteAgentAct)
		{
			g_pLinkage->GetGUILinkage()->RemoveTopMenuItem(m_pGenerateRemoteAgentAct);

			m_pCommandsCat->RemoveAction(m_pGenerateRemoteAgentAct);
			m_pGenerateRemoteAgentAct = NULL;
		}
		g_pLinkage->RemoveActionCategory(m_pCommandsCat);
		m_pCommandsCat = NULL;
	}

	if (m_pImportSAM)
	{
		g_pLinkage->RemoveComponent(m_pImportSAM);
		delete m_pImportSAM;
		m_pImportSAM = NULL;
	}
	if (m_pImportSAMGUI)
	{
		g_pLinkage->RemoveComponent(m_pImportSAMGUI);
		delete m_pImportSAMGUI;
		m_pImportSAMGUI = NULL;
	}
	if (m_pImportNTDS)
	{
		g_pLinkage->RemoveComponent(m_pImportNTDS);
		delete m_pImportNTDS;
		m_pImportNTDS = NULL;
	}
	if (m_pImportNTDSGUI)
	{
		g_pLinkage->RemoveComponent(m_pImportNTDSGUI);
		delete m_pImportNTDSGUI;
		m_pImportNTDSGUI = NULL;
	}
	if (m_pImportPWDump)
	{
		g_pLinkage->RemoveComponent(m_pImportPWDump);
		delete m_pImportPWDump;
		m_pImportPWDump = NULL;
	}
	if (m_pImportPWDumpGUI)
	{
		g_pLinkage->RemoveComponent(m_pImportPWDumpGUI);
		delete m_pImportPWDumpGUI;
		m_pImportPWDumpGUI = NULL;
	}
	if (m_pImportWinRemote)
	{
		g_pLinkage->RemoveComponent(m_pImportWinRemote);
		delete m_pImportWinRemote;
		m_pImportWinRemote = NULL;
	}
	if (m_pImportWinRemoteGUI)
	{
		g_pLinkage->RemoveComponent(m_pImportWinRemoteGUI);
		delete m_pImportWinRemoteGUI;
		m_pImportWinRemoteGUI = NULL;
	}
	if (m_pImportWinLocal)
	{
		g_pLinkage->RemoveComponent(m_pImportWinLocal);
		delete m_pImportWinLocal;
		m_pImportWinLocal = NULL;
	}
	if (m_pImportWinLocalGUI)
	{
		g_pLinkage->RemoveComponent(m_pImportWinLocalGUI);
		delete m_pImportWinLocalGUI;
		m_pImportWinLocalGUI = NULL;
	}

	if (m_pWindowsImportSettings)
	{
		g_pLinkage->RemoveComponent(m_pWindowsImportSettings);
		delete m_pWindowsImportSettings;
		m_pWindowsImportSettings = NULL;
	}


	return true;
}


