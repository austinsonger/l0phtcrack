#include<stdafx.h>


CLC7ImportUnixPlugin::CLC7ImportUnixPlugin()
{TR;

	m_pImportShadow = NULL;
	m_pImportShadowGUI = NULL;
	m_pImportUnixSSH = NULL;
	m_pImportUnixSSHGUI = NULL;
	m_pImportCat = NULL;
	m_pFileCat = NULL;
	m_pRemoteCat = NULL;
	m_pShadowImportAct = NULL;
	m_pSSHImportAct = NULL;
}

CLC7ImportUnixPlugin::~CLC7ImportUnixPlugin()
{TR;
}

ILC7Interface *CLC7ImportUnixPlugin::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Plugin")
	{
		return this;
	}
	return NULL;
}

QUuid CLC7ImportUnixPlugin::GetID()
{TR;
	return UUID_IMPORTUNIXPLUGIN;
}

QList<QUuid> CLC7ImportUnixPlugin::GetInternalDependencies()
{TR;
	return QList<QUuid>();
}

bool CLC7ImportUnixPlugin::Activate()
{	

#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		return false;
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		WSACleanup();
		return false;
	}
#endif

	m_pImportShadow = new CImportShadow();
	m_pImportShadowGUI = new CImportShadowGUI();
	m_pImportUnixSSH = new CImportUnixSSH();
	m_pImportUnixSSHGUI = new CImportUnixSSHGUI();

	bool bSuccess=true;
	bSuccess &= g_pLinkage->AddComponent(m_pImportShadow);
	bSuccess &= g_pLinkage->AddComponent(m_pImportShadowGUI);
	bSuccess &= g_pLinkage->AddComponent(m_pImportUnixSSH);
	bSuccess &= g_pLinkage->AddComponent(m_pImportUnixSSHGUI);

	m_pImportCat=g_pLinkage->CreateActionCategory("import","Import","Import password hashes to audit");
	m_pFileCat = m_pImportCat->CreateActionCategory("file", "File", "Import from file");
	m_pRemoteCat = m_pImportCat->CreateActionCategory("remote", "Remote", "Import from remote machine");

	m_pShadowImportAct = m_pFileCat->CreateAction(m_pImportShadowGUI->GetID(), "gui", QStringList(),
		"Import from Linux/BSD/Solaris/AIX passwd/shadow file",
		"Import password hashes from a Unix-like passwd and/or shadow format file.");

	m_pSSHImportAct = m_pRemoteCat->CreateAction(m_pImportUnixSSHGUI->GetID(), "gui", QStringList(),
		"Import from Linux/BSD/Solaris/AIX system over SSH",
		"Import password hashes remotely from a Unix-like system over SSH.");

	// Register account types we can import
	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);

#define REGISTER_HASH_TYPE(TYPE, NAME, PLATFORM, DESC) \
	passlink->RegisterHashType(FOURCC(TYPE), NAME, DESC, "import", PLATFORM, GetID());

	REGISTER_HASH_TYPE(HASHTYPE_UNIX_DES, "DES/Crypt", "Unix", "UNIX DES Crypt");
	REGISTER_HASH_TYPE(HASHTYPE_UNIX_MD5, "MD5/Crypt", "Unix", "Linux/BSD/Solaris MD5 Crypt");
	REGISTER_HASH_TYPE(HASHTYPE_UNIX_BLOWFISH, "BF/Crypt", "Unix", "Linux/BSD/Solaris Blowfish Crypt");
	REGISTER_HASH_TYPE(HASHTYPE_UNIX_SHA256, "SHA256/Crypt", "Unix", "Linux/BSD/Solaris SHA256 Crypt");
	REGISTER_HASH_TYPE(HASHTYPE_UNIX_SHA512, "SHA512/Crypt", "Unix", "Linux/BSD/Solaris SHA512 Crypt");
	REGISTER_HASH_TYPE(HASHTYPE_AIX_MD5, "SMD5/AIX","AIX", "AIX Salted MD5");
	REGISTER_HASH_TYPE(HASHTYPE_AIX_SHA1, "SSHA1/AIX", "AIX", "AIX Salted SHA-1");
	REGISTER_HASH_TYPE(HASHTYPE_AIX_SHA256, "SSHA256/AIX", "AIX", "AIX Salted SHA-256");
	REGISTER_HASH_TYPE(HASHTYPE_AIX_SHA512, "SSHA512/AIX", "AIX", "AIX Salted SHA-512");

	if(!bSuccess)
	{
		Deactivate();
		return false;
	}

	return true;
}

bool CLC7ImportUnixPlugin::Deactivate()
{TR;
	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	if (passlink)
	{
#define UNREGISTER_HASH_TYPE(TYPE) \
	passlink->UnregisterHashType(FOURCC(TYPE), "import", GetID());

		UNREGISTER_HASH_TYPE(HASHTYPE_UNIX_DES);
		UNREGISTER_HASH_TYPE(HASHTYPE_UNIX_MD5);
		UNREGISTER_HASH_TYPE(HASHTYPE_UNIX_BLOWFISH);
		UNREGISTER_HASH_TYPE(HASHTYPE_UNIX_SHA256);
		UNREGISTER_HASH_TYPE(HASHTYPE_UNIX_SHA512);
		UNREGISTER_HASH_TYPE(HASHTYPE_AIX_MD5);
		UNREGISTER_HASH_TYPE(HASHTYPE_AIX_SHA1);
		UNREGISTER_HASH_TYPE(HASHTYPE_AIX_SHA256);
		UNREGISTER_HASH_TYPE(HASHTYPE_AIX_SHA512);
	}

	if (m_pImportCat)
	{
		if (m_pFileCat)
		{
			if (m_pShadowImportAct)
			{
				m_pFileCat->RemoveAction(m_pShadowImportAct);
				m_pShadowImportAct = NULL;
			}
			m_pImportCat->RemoveActionCategory(m_pFileCat);
			m_pFileCat = NULL;
		}
		if (m_pRemoteCat)
		{
			if (m_pSSHImportAct)
			{
				m_pRemoteCat->RemoveAction(m_pSSHImportAct);
				m_pSSHImportAct = NULL;
			}
			m_pImportCat->RemoveActionCategory(m_pRemoteCat);
			m_pRemoteCat = NULL;
		}

		g_pLinkage->RemoveActionCategory(m_pImportCat);
		m_pImportCat = NULL;
	}

	if (m_pImportShadow)
	{
		g_pLinkage->RemoveComponent(m_pImportShadow);
		delete m_pImportShadow;
		m_pImportShadow = NULL;
	}
	if (m_pImportShadowGUI)
	{
		g_pLinkage->RemoveComponent(m_pImportShadowGUI);
		delete m_pImportShadowGUI;
		m_pImportShadowGUI = NULL;
	}
	if (m_pImportUnixSSH)
	{
		g_pLinkage->RemoveComponent(m_pImportUnixSSH);
		delete m_pImportUnixSSH;
		m_pImportUnixSSH = NULL;
	}
	if (m_pImportUnixSSHGUI)
	{
		g_pLinkage->RemoveComponent(m_pImportUnixSSHGUI);
		delete m_pImportUnixSSHGUI;
		m_pImportUnixSSHGUI = NULL;
	}

#ifdef _WIN32
	WSACleanup();
#endif

	return true;
}


